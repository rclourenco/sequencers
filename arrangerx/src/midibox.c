#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <panel.h>
#include <pthread.h>
#include "iniparser.h"
#include "midi_pattern.h"

#include "../../midi_lib/midi_lib.h"


typedef struct {
	char *midiin;
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
    printf("Cannot play MIDI Files with SMTPE timing\n");
    return;
  }
  unsigned long quarter_interval = 500000;
  if(pat->tempo)
    quarter_interval = pat->tempo;

  printf("Quarter Interval %8lu\n", quarter_interval);
  unsigned long ticks_interval = quarter_interval/pat->ticks_quarter;
  printf("Ticks Interval %8lu\n", ticks_interval);

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
      printf("Ticks Interval %8lu => %8ld \n", ticks_interval, adiff-adiff_r);
    }

    //unsigned long adiff2 = (tick+1)*ticks_interval;

    if( osec != secs ) {
      printf("BPM: %3lu, Time: %02u:%02u:%02u %8lu\n", 60000000L/quarter_interval, (unsigned int)(secs/3600)%24,(unsigned int) (secs/60)%60, (unsigned int)secs%60, adiff);
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

    otick = tick;
    o_adiff = adiff;
  }

}

typedef void (*PlayingStatusCB)(void *data, int pat, unsigned long secs);

volatile sig_atomic_t x = 0;
volatile sig_atomic_t player_on = 0;

void play_sequence(MidiPattern **patlist, int n, PlayingStatusCB stcb, void *stcb_data)
{

  if (!n)
	  return;

  int pt = 0;
  MidiPattern *pat = patlist[pt++];
  rewind_pattern(pat);

  if(pat->ticks_quarter == 0) {
    return;
  }
 
  unsigned long quarter_interval = 500000;
  if(pat->tempo)
    quarter_interval = pat->tempo;
  unsigned long ticks_interval = quarter_interval/pat->ticks_quarter;

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

  if (stcb)
	  stcb(stcb_data, pt-1, 0);

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

    if (pat->ticks_quarter==0)
	    break;
    if(pat->tempo && pat->tempo != quarter_interval) {
      quarter_interval = pat->tempo;
      ticks_interval = quarter_interval/pat->ticks_quarter;
      // rebase
      o_usec_el = usec_el;
      btick = tick;
      bref += adiff;
    }

    //unsigned long adiff2 = (tick+1)*ticks_interval;

    if( osec != secs ) {
	  if (stcb)
	  	stcb(stcb_data, pt-1, secs);

      osec = secs;
    }

    if (x==2) {
	    break;
    }

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

    otick = tick;
    o_adiff = adiff;

    if (done) {
	if (x==1 || x==2)
		break;
	if (pt<n) {
		done=0;
		if (x!=3)
			pat=patlist[pt++];
		else
			x=0;
		rewind_pattern(pat);
		if(pat->tempo)
                   quarter_interval = pat->tempo;
		if (pat->ticks_quarter==0)
			break;
		ticks_interval = quarter_interval/pat->ticks_quarter;
		otick = 0;
	       	m_otick=0;
                btick = 0;
		o_usec_el = usec_el;

		if (stcb)
	  		stcb(stcb_data, pt-1, 0);

	} else {
    		break;
	}
    }


  }

  if (stcb)
	stcb(stcb_data, -1, 0);
}

typedef struct {
	MidiPattern **list;
	int len;
}PlayerData;

void update_cb(void *data, int pat, unsigned long secs)
{
	if (pat!=-1) {
		printf("\rPattern %02u Time: %02u:%02u:%02u  [<ESC>: Next Pat, <SP>: Quit]", pat, (unsigned int)(secs/3600)%24,(unsigned int) (secs/60)%60, (unsigned int)secs%60);
		fflush(stdout);
		return;
	}
	printf("\r\nDone!\r\n");
}

void *player_th_func(void *data)
{
	PlayerData *pdata = (PlayerData *)data;
	play_sequence(pdata->list, pdata->len, update_cb, pdata);
	player_on = 0;
	pthread_exit(NULL);
	return NULL;	
}

pthread_t player_th;
pthread_attr_t th_attr;

PlayerData player_data;

void start_playing(MidiPattern **list, int len)
{
	player_data.list = list;
	player_data.len = len;
        player_on = 1;
	pthread_attr_init(&th_attr);
	pthread_attr_setdetachstate(&th_attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&player_th, &th_attr, player_th_func, (void *) &player_data);
	pthread_attr_destroy(&th_attr);
}

void player_wait()
{
	pthread_join(player_th, NULL);	
}

volatile sig_atomic_t reader_on = 0;

void *midireader_th_func(void *data)
{
	MidiParser mp;
	MidiEvent  me;
	midiParserInit(&mp);

	printf("READER STARTED!!!\n");
	while(reader_on) {
		int input = midi_in();
		if (input >= 0) {
			unsigned char msg = input;
			if (midiParser(&mp, input, &me)) {
				switch(me.cmd) {
					case 0x82:
					case 0x92:
					case 0xA2:
					case 0xB2:
					case 0xE2:
						msg = me.cmd;midi_out(&msg,1);
						msg = me.p1; midi_out(&msg,1);
						msg = me.p2; midi_out(&msg,1);
						break;
					case 0xC2:
					case 0xD2:
						msg = me.cmd;midi_out(&msg,1);
						msg = me.p1; midi_out(&msg,1);
						break;
					case 0xFA: x = 3; break;
					case 0xFB: x = 2; break;
					case 0xFC: x = 1; break;
				}
			}
			
			//printf("MIDI IN: %02X\n", input);
			continue;
		}
		usleep(1000);
	}
	printf("READER EXIT\n");
	reader_on = 0;
	pthread_exit(NULL);
	return NULL;	
}

pthread_t reader_th;
void midi_reader_start()
{
	reader_on = 1;
	pthread_attr_init(&th_attr);
	pthread_attr_setdetachstate(&th_attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&reader_th, &th_attr, midireader_th_func, NULL);
	pthread_attr_destroy(&th_attr);
}

void midi_reader_shutdown()
{
	reader_on = 0; // request midireader thread quit
	pthread_join(reader_th, NULL);
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

void free_patterns()
{
	if (totalpatterns==0)
		return;
	int i;
	for(i=0;i<totalpatterns;i++) {
		midi_pattern_free(&list[i]);		
	}
	totalpatterns=0;

}
int load_song(char *filename);

int load_and_play(char *filename)
{
	int r = load_song(filename);
	if(!r)
		return r;
	int i;

  	for(i=0;i<totalpatterns;i++) {
  		printf("P %02u: %-30s\r\n", i, list[i]->filename);
  	}

	printf("Press Enter to Start Playing!\n");
  	getchar();

  	x=0;
  	start_playing(list, totalpatterns);
  	do {
  		char t = getchar();
		if (player_on==0)
			break;
		switch(t) {
			case 'n': x=1; break;
			case 'q': x=2; break;
			case 'r':
			case 'R': x=3; break;
		}
  	} while(1);
  	player_wait();
  	midi_panic();
	return 1;
}

int main(int argc, char **argv)
{
  if( argc < 2) {
    return 1;
  }
  char *ext = NULL;
  int error=0;

  load_config(&config);

  printf("==> %s\n", config.midiout);
  printf("==> %s\n", config.midiin);

  printf("==> %s\n", config.songs);

  char mididevs[200];
  strcpy(mididevs, "");
  if (config.midiout)
  	strcpy(mididevs, config.midiout);

  if (config.midiin) {
	strcat(mididevs, ";");
  	strcat(mididevs, config.midiin);
  }

  midi_set_input_blocking(0);
  midi_port_install(mididevs);
  midi_reset();
  signal(SIGTERM, terminate_handler);
  signal(SIGQUIT, terminate_handler);
  signal(SIGINT, terminate_handler);
  signal(SIGHUP, terminate_handler);
  atexit(midi_panic);

  //atexit(apshutdown);

  //create_pattern_window();
  //create_player_window();
  //update_panels();
  //doupdate();

//  hide_panel(player_panel);

  midi_reader_start();
  load_and_play(argv[1]);
  
  getchar();
  //hide_panel(player_panel);
  midi_reader_shutdown();

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
	if (!strcmp(key, "midiin")) {
		if (cl->cfg->midiin!=NULL)
			return 0;
		char *r = index(value, ';');
		if (!r)
			return -1;

		int l = strlen(value)+4; /* strlen("out:") + 1 */

		cl->cfg->midiin = (char *)malloc(l);
		if (!cl->cfg->midiin)
			return -2;

		strncpy(cl->cfg->midiin, value, r-value+1);
		strcpy(&cl->cfg->midiin[r-value+1], "in:");
		strcat(cl->cfg->midiin, r+1);

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
	        printf("Part name: %s\r\n", key);	
		return 0;
	}

	if (!strcmp(key, "filename")) {
		if (sl->pos!=totalpatterns) 
			return 0;
		list[totalpatterns] = midi_pattern_load(value);
          	if (list[totalpatterns]) {
            		printf("Loaded %s\r\n", value);
            		totalpatterns++;
          	} else {
            		printf("Error loading %s\r\n", value);
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

    free_patterns();
    totalpatterns = 0;
    if (!strcasecmp(ext, ".mid" )) {
    	list[0] = midi_pattern_load(filename);
   	if( list[0] ) {
      	    printf("Import Ok\r\n");
      	    totalpatterns = 1;
    	}
	return totalpatterns;
    }

	if (!strcasecmp(ext, ".lst")) {
        	FILE *fp;
		printf("%s\r\n", filename);
 		if(fp=fopen(filename,"rt")) {
      			while(!feof(fp)) {
        			char buffer[200];
        			int ch;
        			int l=0;
        			while( (ch=fgetc(fp)) != '\n' && ch != EOF ) {
          				if(l<199) buffer[l++] = ch;
        			}
        			buffer[l]='\0';
        			printf("Line %s\r\n", buffer);
        			if( (ext = rindex(buffer,'.')) && !strcasecmp(ext, ".mid" ) && totalpatterns < MAXPATTERNS ) {
          				list[totalpatterns] = midi_pattern_load(buffer);
          				if(list[totalpatterns]) {
            					printf("Loaded %s\r\n", buffer);
            					totalpatterns++;
          				} else {
            					printf("Error loading %s\r\n", buffer);
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
