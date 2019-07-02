#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <panel.h>
#include "iniparser.h"
#include "midi_pattern.h"

#include "../../midi_lib/midi_lib.h"


typedef struct {
	char *midiout;
	char *songs;
} MidiBoxConfig;

volatile sig_atomic_t a;

void meta_midi_event(MidiEventNode *en, MidiPattern *pat)
{
  if(en->type != meMeta)
    return;
  if(en->t.meta.type == 0x51 && en->t.meta.length == 3) {
    pat->tempo = (en->data[0]<<16)+(en->data[1]<<8)+en->data[2];
//    printf("Tempo Change Request: %d %d %d => %lu\n", en->data[0], en->data[1], en->data[2], pat->tempo);
  }
}

void execute_midi_event(MidiEventNode *en)
{
  unsigned char msg[3];
  switch(en->type)
  {
    case meCV:{
      msg[0] = 0x70 + (en->t.vc.type << 4) + (en->t.vc.channel & 0xF);
      if(en->datalen==1) {
        msg[1]=en->data[0];
        midi_out(msg,2);
      }
      else if(en->datalen==2) {
        msg[1]=en->data[0];
        msg[2]=en->data[1];
        midi_out(msg,3);
      }
    }
    break;
    case meCVRunning:
      {
        char c=0;
        msg[c++] = 0x70 + (en->t.vc.type << 4) + (en->t.vc.channel & 0xF);
        if(en->datalen==1) {
          msg[c++]=en->data[0];
          midi_out(msg,c);
        }
        else if(en->datalen==2) {
          msg[c++]=en->data[0];
          msg[c++]=en->data[1];
          midi_out(msg,c);
        }
      }
    break;
  }
}

int play_pattern(MidiPattern *pat, unsigned long tick)
{
  MidiTrackNode *current = pat->tracks;
  int playing = 0;
  int i = 0;
  while(current) {
    i++;
    while(current->playing && current->playing->time <= tick) {
/*      printf("Track %d Fire Midi Events @ %lu\n", i, tick);*/
      if(current->playing->type == meMeta)
        meta_midi_event(current->playing, pat); /*Sequencer Meta Event*/
      else
        execute_midi_event(current->playing); /*Synth Event*/

      current->playing = current->playing->next;
    }

    if(current->playing)
      playing++;

    current = current->next;
  }

  return !playing;
}

void rewind_pattern(MidiPattern *pat)
{
  MidiTrackNode *current = pat->tracks;
  int alldone = 0;
  while(current) {
    current->playing = current->events;
    current = current->next;
  }

}

WINDOW
	*player_win = NULL,
	*pattern_win = NULL;
PANEL
	*player_panel = NULL,
	*pattern_panel = NULL;

void playing_loop(MidiPattern *pat)
{
  if(pat->ticks_quarter == 0) {
    mvwprintw(player_win, 3, 2, "Cannot play MIDI Files with SMTPE timing\n"); wrefresh(player_win);
    return;
  }
  unsigned long quarter_interval = 500000;
  if(pat->tempo)
    quarter_interval = pat->tempo;
  mvwprintw(player_win, 3, 2, "Quarter Interval %8lu", quarter_interval); wrefresh(player_win);
  unsigned long ticks_interval = quarter_interval/pat->ticks_quarter;
  mvwprintw(player_win, 6, 2, "Ticks Interval %8lu", ticks_interval); wrefresh(player_win);

  struct timespec tp;
  unsigned long usec_el=0, o_usec_el=0,adiff=0, o_adiff=0, m_adiff=0;

  clock_gettime(CLOCK_REALTIME, &tp);
  o_usec_el = tp.tv_nsec/1000 + (tp.tv_sec%1000) * 1000000;

  unsigned long i;
  unsigned long osec = 10000;
  int done = 0;
  unsigned long otick = 0, m_otick=0;
  unsigned long btick = 0;
  int first = 1;
  int bref = 0;
  while(!done)
  {
    clock_gettime(CLOCK_REALTIME, &tp);
    usec_el = tp.tv_nsec/1000 + (tp.tv_sec%1000) * 1000000;
    if (first) {
	    o_usec_el = usec_el;
	    adiff = 0;
	    first = 0;
    }
    else {
    	if(usec_el < o_usec_el)
      		adiff = abs(1000000000+usec_el - o_usec_el);
    	else
      		adiff = abs(usec_el - o_usec_el);
    }

    unsigned long tick = adiff/ticks_interval+btick;
    unsigned long adiff_r = (tick-btick) * ticks_interval;

    unsigned long adiff2 = (tick+1)*ticks_interval;

    done = play_pattern(pat,tick);

    unsigned long secs = (adiff+bref)/1000000;

    if(pat->tempo && pat->tempo != quarter_interval) {
      quarter_interval = pat->tempo;
      ticks_interval = quarter_interval/pat->ticks_quarter;
      // rebase
      o_usec_el = usec_el;
      btick = tick;
      bref += adiff;
      mvwprintw(player_win, 6, 2, "Ticks Interval %8lu => %8ld ", ticks_interval, adiff-adiff_r); wrefresh(player_win);
    }

    //unsigned long adiff2 = (tick+1)*ticks_interval;

    if( osec != secs ) {
      mvwprintw(player_win, 4, 2, "BPM: %3lu, Time: %02u:%02u:%02u %8lu", 60000000L/quarter_interval, (unsigned int)(secs/3600)%24,(unsigned int) (secs/60)%60, (unsigned int)secs%60, adiff);
      wrefresh(player_win);
      //fflush(stdout);
      osec = secs;
    }
    if(done)
      break;
    unsigned long wtime = ticks_interval-(adiff-adiff_r);
    if (wtime>0 && wtime <= ticks_interval)
	    usleep(wtime);

  /*  if (adiff2 > adiff) {
    	usleep(adiff2-adiff);
    }
    */

   if (adiff-o_adiff > m_adiff)
	   m_adiff = adiff-o_adiff;

   if (tick - otick > m_otick)
	   m_otick = tick - otick;

    mvwprintw(player_win, 5, 2, "%8lu   %8ld", m_otick, m_adiff);
    wrefresh(player_win);
   
    otick = tick;
    o_adiff = adiff;
  }

//  mvwprintw(player_win, 5, 2, "Done.");
  wrefresh(player_win);
}

void midi_reset()
{
  int channel;
  unsigned char msg[3];
  for(channel=0;channel<15;channel++) {
    // All Notes Off
    msg[0] = 0xB0 + channel;
    msg[1] = 123;
    msg[2] = 0;
    midi_out(msg,3);
    // Reset Controllers
    msg[0] = 0xB0 + channel;
    msg[1] = 121;
    msg[2] = 0;
    midi_out(msg,3);
    
    // Reset Balance
    msg[0] = 0xB0 + channel;
    msg[1] = 8;
    msg[2] = 64;
    midi_out(msg,3);

  }


}

void midi_panic()
{
  int channel;
  unsigned char msg[3];
  for(channel=0;channel<15;channel++) {
    msg[0] = 0xB0 + channel;
    msg[1] = 123;
    msg[2] = 0;
    midi_out(msg,3);
  }
}


void terminate_handler(int n)
{
  printf("Terminate Handler input: %d\n", n);
  exit(2);
}

#define MAXPATTERNS 10
int totalpatterns = 0;
MidiPattern *list[MAXPATTERNS];

void apshutdown();

void apshutdown()
{
	if (player_win != NULL)
		delwin(player_win);
	endwin();
	return;
}

WINDOW *create_player_window()
{
	player_win = newwin(8, 40, 10, 10);
	//box(player_win, 0 , 0);

	wborder(player_win, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(player_win);

	player_panel = new_panel(player_win);
	return player_win;
}

WINDOW *create_pattern_window()
{
	pattern_win = newwin(8, 40, 1, 1);
	//box(player_win, 0 , 0);

	wborder(pattern_win, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(pattern_win);

	pattern_panel = new_panel(pattern_win);
	return pattern_win;
}

int load_config(MidiBoxConfig *cfg);

MidiBoxConfig config;

int load_song(char *filename);

int main(int argc, char **argv)
{
  if( argc < 2) {
    return 1;
  }
  char *ext = NULL;
  int error=0;

  load_config(&config);

  printf("==> %s\n", config.midiout);
  printf("==> %s\n", config.songs);

  // TODO free config
  getchar();

  load_song(argv[1]);

  getchar();

  if(error) {
    printf("Aborting....\n");
    exit(2);
  }

  midi_port_install(config.midiout);
  midi_reset();
  signal(SIGTERM, terminate_handler);
  signal(SIGQUIT, terminate_handler);
  signal(SIGINT, terminate_handler);
  signal(SIGHUP, terminate_handler);
  atexit(midi_panic);

  initscr();
  atexit(apshutdown);

  create_pattern_window();
  create_player_window();
  update_panels();
  doupdate();

  hide_panel(player_panel);
  int i;

  for(i=0;i<totalpatterns;i++) {
  	mvwprintw(pattern_win, i+1, 2, "%-30s", list[i]->filename);
	wrefresh(pattern_win);
  }

  getch();

  show_panel(player_panel);

  for(i=0;i<totalpatterns;i++) {
    doupdate();
    mvwprintw(player_win, 2, 2, "Playing %-25s", list[i]->filename);
    wrefresh(player_win);
    rewind_pattern(list[i]);
    playing_loop(list[i]);
  }
  mvwprintw(player_win, 5, 2, "Done!");
  wrefresh(player_win);

  getch();
  hide_panel(player_panel);

}

struct _config_loader {
	int current_section;
	MidiBoxConfig *cfg;
};

int config_setter(void *data, char *key, char *value)
{
	struct _config_loader *cl = (struct _config_loader *) data;

	if (key==NULL)
		return 0;

	if (value==NULL) {

		if (!strcmp(key, "main")) {
			cl->current_section = 0;
		} else {
			cl->current_section = -1;
		}

		return 0;
	}

	if (cl->current_section != 0)
		return 0;

	if (!strcmp(key, "midiout")) {
		if (cl->cfg->midiout!=NULL)
			return 0;
		char *r = index(value, ';');
		if (!r)
			return -1;

		int l = strlen(value)+5; /* strlen("out:") + 1 */

		cl->cfg->midiout = (char *)malloc(l);
		if (!cl->cfg->midiout)
			return -2;

		strncpy(cl->cfg->midiout, value, r-value+1);
		strcpy(&cl->cfg->midiout[r-value+1], "out:");
		strcat(cl->cfg->midiout, r+1);

		return 0;
	}

	if (!strcmp(key, "songs")) {
		if (cl->cfg->songs!=NULL)
			return 0;

		cl->cfg->songs = strdup(value);
		return 0;
	}

	return 0;
}

int load_config(MidiBoxConfig *cfg)
{
	struct _config_loader config_loader;
	config_loader.current_section = 0;
	config_loader.cfg = cfg;

	cfg->midiout=NULL;
	cfg->songs=NULL;

	return ini_parse("midibox.conf", &config_loader, config_setter);
}

struct _sequence_loader {
	int pos;
};

int sequence_setter(void *data, char *key, char *value);

int sequence_setter(void *data, char *key, char *value)
{
	struct _sequence_loader *sl = (struct _sequence_loader *) data;

	if (key==NULL)
		return 0;

	if (value==NULL) {
		sl->pos++;
	        printf("Part name: %s\n", key);	
		return 0;
	}

	if (!strcmp(key, "filename")) {
		if (sl->pos!=totalpatterns) 
			return 0;
		list[totalpatterns] = midi_pattern_load(value);
          	if (list[totalpatterns]) {
            		printf("Loaded %s\n", value);
            		totalpatterns++;
          	} else {
            		printf("Error loading %s\n", value);
          	}

		return 0;
	}


	return 0;
}

int load_song(char *filename)
{
    char *ext = rindex(filename, '.');
    if (!ext)
	    return 0;

    totalpatterns = 0;
    if (!strcasecmp(ext, ".mid" )) {
    	list[0] = midi_pattern_load(filename);
   	if( list[0] ) {
      	    printf("Import Ok\n");
      	    totalpatterns = 1;
    	}
	return totalpatterns;
    }

	if (!strcasecmp(ext, ".lst")) {
        	FILE *fp;
		printf("%s\n", filename);
 		if(fp=fopen(filename,"rt")) {
      			while(!feof(fp)) {
        			char buffer[200];
        			int ch;
        			int l=0;
        			while( (ch=fgetc(fp)) != '\n' && ch != EOF ) {
          				if(l<199) buffer[l++] = ch;
        			}
        			buffer[l]='\0';
        			printf("Line %s\n", buffer);
        			if( (ext = rindex(buffer,'.')) && !strcasecmp(ext, ".mid" ) && totalpatterns < MAXPATTERNS ) {
          				list[totalpatterns] = midi_pattern_load(buffer);
          				if(list[totalpatterns]) {
            					printf("Loaded %s\n", buffer);
            					totalpatterns++;
          				} else {
            					printf("Error loading %s\n", buffer);
          				}
        			}
      			}
      			fclose(fp);
    		}
    		return totalpatterns;
	}

	if (!strcasecmp(ext, ".seq")) {
		struct _sequence_loader sequence_loader;
		sequence_loader.pos = -1;
		totalpatterns = 0;
		ini_parse(filename, &sequence_loader, sequence_setter);
		return totalpatterns;
	}
	return 0;
}
