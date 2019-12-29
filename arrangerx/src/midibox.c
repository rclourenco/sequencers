#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>
#include <libgen.h>
#include <getopt.h>
#include "iniparser.h"
#include "midi_pattern.h"

#include "../../midi_lib/midi_lib.h"

void set_midi_opt(char **opt_r, char *direction, char *value)
{
	if (opt_r == NULL)
		return;
	if (*opt_r != NULL)
		return;

	char *r = index(value, ';');

	if (!r)
		r = index(value, ':');

	if (!r)
		return;

	int l = strlen(value)+strlen(direction)+2; /* strlen("direction") + strlen(":") + '\0' */

	*opt_r = (char *)malloc(l);
	if (!*opt_r)
		return;

	strncpy(*opt_r, value, r-value+1);

	strcpy(&(*opt_r)[r-value+1], direction);
	strcat(*opt_r, ":");
	strcat(*opt_r, r+1);
}

typedef struct {
	char *midiin;
	char *midiout;
	char *songs;
	char *filepath;

	int cmd_channel;
} MidiBoxConfig;

void reset_config(MidiBoxConfig *cfg)
{
	cfg->midiin = NULL;
	cfg->midiout = NULL;
	cfg->songs = NULL;
	cfg->filepath = NULL;
	cfg->cmd_channel = 0;
}


void load_config_from_args(MidiBoxConfig *cfg, int argc, char **argv)
{
	static struct option long_options[] = {
		{"in", required_argument, 0, 1},
		{"out", required_argument, 0, 2},
		{0, 0, 0, 0}
	};

	while(1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "i:o:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
			case 1:
				set_midi_opt(&cfg->midiin, "in", optarg);
				break;
			case 2:
				set_midi_opt(&cfg->midiout, "out", optarg);
				break;
			default:
				printf("Code: %u\n", c);
		}
	}

	printf("%u %u\n", optind, argc);
	int i;
	for(i=0;i<argc;i++) {
		printf("%u => %s\n", i, argv[i]);
	}

	if (optind < argc) {
		if (cfg->filepath==NULL) {
			cfg->filepath = argv[optind++];
		}
	}
}

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

typedef void (*PlayingStatusCB)(void *data, int pat, unsigned long secs, unsigned long ticks);

volatile sig_atomic_t x = 0;
volatile sig_atomic_t player_on = 0;

int nextPattern(MidiPattern **patlist, int current, int action)
{
	if (current<0)
		return 0;

	int default_next = current + 1;
	MidiPattern *pat = patlist[current];
	if (pat->last)
		return -1;
	if (pat->next)
		default_next = pat->next - 1;

	if (action < 10)
		return default_next;

	action -= 10;

	if (action<MAX_ACTIONS) {
		int next = pat->actions[action] & 0xFF;
		if (next) 
			return next - 1;
	}

	return default_next;
}

struct player_status {
	int loop;
	int pattern;
};

void play_sequence(MidiPattern **patlist, int n, PlayingStatusCB stcb, void *stcb_data)
{

  	if (!n)
		 return;

	struct player_status plst;
	plst.loop = 0;
	plst.pattern = 0;

	MidiPattern *pat = patlist[plst.pattern];
	rewind_pattern(pat);

	if(pat->ticks_quarter == 0) {
		return;
	}
 
	unsigned long quarter_interval = 500000;
	if (pat->tempo)
    		quarter_interval = pat->tempo;
  	unsigned long ticks_interval = quarter_interval/pat->ticks_quarter;

  	struct timespec tp;
  	unsigned long usec_el=0, o_usec_el=0, adiff=0;

	unsigned long i;
	int done = 0;
	unsigned long otick = 0;
	unsigned long btick = 0;
 	int first = 1;
 	int bref = 0;

	int wait = 1;
	plst.loop = pat->loop;

 	if (stcb)
		stcb(stcb_data, plst.pattern, 0, 0);

	while(!done)
	{
		if (wait && pat->wait) {
			wait = 0;
			while(!x) {
				usleep(500);
			}
			first = 1;
		}
		clock_gettime(CLOCK_REALTIME, &tp);
		usec_el = tp.tv_nsec/1000 + (tp.tv_sec%1000) * 1000000;
		if (first) {
	    		o_usec_el = usec_el;
	    		adiff = 0;
	    		first = 0;
			otick = 0;
			btick = 0;
			bref=0;
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
		if (pat->tempo && pat->tempo != quarter_interval) {
			quarter_interval = pat->tempo;
 			ticks_interval = quarter_interval/pat->ticks_quarter;
			// rebase
			o_usec_el = usec_el;
			btick = tick;
			bref += adiff;
    		}

    //unsigned long adiff2 = (tick+1)*ticks_interval;

		if ( tick != btick ) {
	  		if (stcb)
	  			stcb(stcb_data, plst.pattern, secs, tick);
		}

		if (x==2) {
	    		break;
    		}

		unsigned long wtime = ticks_interval-(adiff-adiff_r);
		if (wtime>0 && wtime <= ticks_interval)
			usleep(wtime);

    		otick = tick;

		if (!done)
			continue;

		if (x==1 || x==2)
			break;

		if (x==21)
			plst.loop = !plst.loop;

		done=0;
		if (x!=3 && !plst.loop) {
			int next = nextPattern(patlist, plst.pattern, x);
			if (next < 0 || next >= n) {
				x=0;
				while (!x) {
					usleep(500);
				}
				if (x==1 || x==2)
					break;
				wait = 0;
				first = 1;
				next = 0;
			}

			pat=patlist[next];
			plst.pattern = next;
			plst.loop = pat->loop;
		}

		if (x>=10 || x==3)
			x=0;
		rewind_pattern(pat);
		if(pat->tempo)
        		quarter_interval = pat->tempo;
		if (pat->ticks_quarter==0)
			break;
		ticks_interval = quarter_interval/pat->ticks_quarter;
		otick = 0;
        	btick = 0;
		o_usec_el = usec_el;

		if (stcb)
	 		stcb(stcb_data, plst.pattern, 0, 0);
	}

  	if (stcb)
		stcb(stcb_data, -1, 0, 0);
}

typedef struct {
	MidiPattern **list;
	int len;

	// player status
	unsigned long seconds;
	int pattern;
	unsigned long ticks;
}PlayerData;

void update_cb(void *data, int pat, unsigned long secs, unsigned long ticks)
{
	PlayerData *pdata = (PlayerData *)data;
	pdata->pattern = pat;
	pdata->seconds = secs;
	pdata->ticks   = ticks;

	if (pat!=-1) {
//		printf("\rPattern %02u Time: %02u:%02u:%02u  [<ESC>: Next Pat, <SP>: Quit]", pat, (unsigned int)(secs/3600)%24,(unsigned int) (secs/60)%60, (unsigned int)secs%60);
//		fflush(stdout);
		return;
	}
//	printf("\r\nDone!\r\n");
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
	player_data.pattern = 0;
	player_data.seconds = 0;
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
	MidiBoxConfig *cfg = (MidiBoxConfig *)data;
	MidiParser mp;
	MidiEvent  me;
	midiParserInit(&mp);

	printf("READER STARTED!!!\n");
	while(reader_on) {
		int input = midi_in();
		if (input >= 0) {
			unsigned char msg = input;
			if (midiParser(&mp, input, &me)) {
				int cmd = me.cmd;
				if (cmd < 0xF0)
					cmd &= 0xF0;
				int channel = me.cmd & 0xF;
				int y = channel == cfg->cmd_channel;

				switch(cmd) {
					case 0x80:
					case 0x90:
					case 0xA0:
					case 0xB0:
					case 0xE0:
						if (cmd==0xB0 && me.p1==16 && y) {
							x = me.p2 + 10;
						}

						msg = me.cmd;midi_out(&msg,1);
						msg = me.p1; midi_out(&msg,1);
						msg = me.p2; midi_out(&msg,1);
						break;
					case 0xC0:
					case 0xD0:
						msg = me.cmd;midi_out(&msg,1);
						msg = me.p1; midi_out(&msg,1);
						break;
					case 0xF3:
						x = me.p1 + 10;
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
void midi_reader_start(MidiBoxConfig *cfg)
{
	reader_on = 1;
	pthread_attr_init(&th_attr);
	pthread_attr_setdetachstate(&th_attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&reader_th, &th_attr, midireader_th_func, cfg);
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
	return;
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
	getchar();
	int r = load_song(filename);
	if(!r)
		return r;
	int i;

  	for(i=0;i<totalpatterns;i++) {
  		printf("P %02u: %-30s\r\n", i, list[i]->filename);
  	}

	int t=0;
	printf("Press Enter to Start Playing (q to skip)! \n");
  	while (t!='q' && t!='\n') {
		t = getchar();
	}
	
	if (t=='q')
		return 2;

  	x=0;
	printf("\x1B[0;32;40m\x1B[2J\x1B[H");
	draw_pattern(list, totalpatterns, 0, 0);
  	start_playing(list, totalpatterns);
  	do {
		fd_set rfds;
		struct timeval tv;
		int retval;
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);

		tv.tv_sec = 0;
		tv.tv_usec = 20000;

		retval = select(1, &rfds, NULL, NULL, &tv);
		
		if (player_data.pattern != -1 ) {
			unsigned long secs = player_data.seconds;
			printf("\x1B[32m\x1B[H");
			draw_pattern(list, totalpatterns, player_data.pattern, player_data.ticks);
			printf("\r\x1B[33mPattern %02u Time: %02u:%02u:%02u  [<ESC>: Next Pat, <SP>: Quit]", player_data.pattern, (unsigned int)(secs/3600)%24,(unsigned int) (secs/60)%60, (unsigned int)secs%60);
			fflush(stdout);
		}

		if (retval==-1 || !retval)
			continue;

  		t = getchar();
		if (player_on==0)
			break;
		switch(t) {
			case 'n': x=1; break;
			case 'q': x=2; break;
			case 'r':
			case 'R': x=3; break;
		}
  	} while (t!='q');
  	player_wait();
  	midi_panic();
	return t=='q' ? 2 : 1;
}

struct termios orig_termios;
void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);
	struct termios raw = orig_termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char **argv)
{
  char *ext = NULL;
  int error=0;

  memset(list, 0, sizeof(MidiPattern *)*MAXPATTERNS);

  reset_config(&config);
  load_config_from_args(&config, argc, argv);

  printf("==> %s\n", config.midiout);
  printf("==> %s\n", config.midiin);

  printf("==> %s\n", config.songs);

  if (config.filepath==NULL) {
	  return 1;
  }

  load_config(&config);

  char mididevs[200];
  strcpy(mididevs, "");
  if (config.midiout)
  	strcpy(mididevs, config.midiout);

  if (config.midiin) {
	strcat(mididevs, ";");
  	strcat(mididevs, config.midiin);
  }

  midi_set_input_blocking(0);
  printf("MidiDevs: %s\n", mididevs);
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

  enableRawMode();
  midi_reader_start(&config);
  load_and_play(config.filepath);
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
		set_midi_opt(&cl->cfg->midiout, "out", value);
		return 0;
	}

	if (!strcmp(key, "midiin")) {
		if (cl->cfg->midiin!=NULL)
			return 0;
		set_midi_opt(&cl->cfg->midiin, "in", value);
		return 0;
	}

	if (!strcmp(key, "songs")) {
		if (cl->cfg->songs!=NULL)
			return 0;

		cl->cfg->songs = strdup(value);
		return 0;
	}

	if (!strcmp(key, "midi_cmd_channel")) {
		cl->cfg->cmd_channel = atoi(value);
	}
	return 0;
}

int load_config(MidiBoxConfig *cfg)
{
	struct _config_loader config_loader;
	config_loader.current_section = 0;
	config_loader.cfg = cfg;

	int r = ini_parse("midibox.conf", &config_loader, config_setter);
	if (cfg->midiin) {
		printf("Set Midi In: %s\n", cfg->midiin);
	}

	if (cfg->midiout) {
		printf("Set Midi Out: %s\n", cfg->midiout);
	}
	return r;
}

struct _sequence_loader {
	int  pos;
	char *sequence_dir;

	char name[PATTERN_MAXNAME+1];
	int  loop;
	int  wait;
	int  last;
	int  default_next;

	unsigned int actions[MAX_ACTIONS];

	int map_count;
	char *next_map[MAXPATTERNS];
	int next_index[MAXPATTERNS];
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
		strcpy(sl->name, key);
		sl->loop = 0;
		sl->wait = 0;
		sl->last = 0;
		sl->default_next = 0;
		memset(sl->actions, 0, sizeof(unsigned int)*MAX_ACTIONS);
		return 0;
	}

	printf(">> KEY %s %s\n", key, value);

	if (sl->pos<0) 
		return 0;

	if (!strcmp(key, "filename")) {
		if (!sl->sequence_dir)
			return 0;
		char *filepath = (char *) malloc(strlen(sl->sequence_dir)+strlen(value)+3);
		if (!filepath)
			return 0;

		if (sl->sequence_dir[0]) {
			printf("SequenceDir %s\n", sl->sequence_dir);
			strcpy(filepath, sl->sequence_dir);
		} else {
			strcpy(filepath, ".");
		}

		strcat(filepath, "/");
		strcat(filepath, value);


		list[totalpatterns] = midi_pattern_load(filepath);

          	if (list[totalpatterns]) {
			MidiPattern *c = list[totalpatterns];
            		printf("Loaded %s\r\n", filepath);
			memcpy(c->name, sl->name, PATTERN_MAXNAME+1);
			c->loop = sl->loop;
			c->wait = sl->wait;
			c->next = sl->default_next;
			c->last = sl->last;
			memcpy(c->actions, sl->actions, sizeof(unsigned int)*MAX_ACTIONS);
            		totalpatterns++;
          	} else {
            		printf("Error loading %s\r\n", filepath);
          	}

		free(filepath);

		return 0;
	}

	if (!strcmp(key, "loop")) {
		if (!strcmp(value, "true") || !strcmp(value,"1")) {
			if (list[sl->pos])
				list[sl->pos]->loop = 1;
			sl->loop = 1;
		}
		return 0;
	}

	if (!strcmp(key, "wait")) {

		if (!strcmp(value, "true") || !strcmp(value,"1")) {
			if (list[sl->pos])
				list[sl->pos]->wait = 1;
			sl->wait = 1;
		}
		return 0;
	}

	if (!strcmp(key, "last")) {

		if (!strcmp(value, "true") || !strcmp(value,"1")) {
			if (list[sl->pos])
				list[sl->pos]->last = 1;
			sl->last = 1;
		}
		return 0;
	}

	if (!strcmp(key, "next")) {
		int i = 0;
		int f = -1;
		for(i=0;i<sl->map_count;i++) {
			if (strcmp(sl->next_map[i], value))
			       continue;
			f = i+1;
			break;	
		}

		if (f==-1) {
			char *t = strdup(value);
			if (!t)
				return -1;
			
			sl->next_map[sl->map_count++] = t;
			f = sl->map_count;
		}

		if (list[sl->pos])
			list[sl->pos]->next = f;

		sl->default_next = f;
		return 0;
	}

	if (!strncmp(key, "action:", 7)) {
		int a = atoi(key+7);
		printf("Action: %d\n", a);
		if (a >= MAX_ACTIONS)
			return 0;

		if (sl->actions[a])
			return 0;
	
		if (list[sl->pos] && list[sl->pos]->actions[a])
			return 0;

		sl->actions[a] = 0x1000;
	
		if (list[sl->pos])
		       	list[sl->pos]->actions[a] = 0x1000;

		if (!strncmp(value, "next:", 5)) {
			int i = 0;
			int f = -1;
			for(i=0;i<sl->map_count;i++) {
				if (strcmp(sl->next_map[i], value+5))
				       continue;
				f = i+1;
				break;	
			}

			if (f==-1) {
				char *t = strdup(value+5);
				if (!t)
					return -1;
			
				sl->next_map[sl->map_count++] = t;
				f = sl->map_count;
			}

			printf("<<<< Action %d, %d\n", a, f-1);
			if (list[sl->pos])
				list[sl->pos]->actions[a] |= f;

			sl->actions[a] |= f;
			return 0;
		}

		if (!strncmp(value, "loop:", 5)) {
			int lflags = 0;

			if (!strcmp(value+5, "on")) {
				lflags = 1;
			}
			else if (!strcmp(value+5, "off")) {
				lflags = 2;
			}
			else if (!strcmp(value+5, "toggle")) {
				lflags = 3;
			}

			if (list[sl->pos])
				list[sl->pos]->actions[a] |= lflags << 8;

			sl->actions[a] |= lflags << 8; 
		}

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
		struct _sequence_loader *sequence_loader = malloc(sizeof(struct _sequence_loader));
		memset(sequence_loader, 0, sizeof(struct _sequence_loader));
		if (!sequence_loader)
			return 0;

		sequence_loader->pos = -1;
		printf("Filename: %s\n", filename);
		char *tmp = strdup(filename);
		if (!tmp)
			return 0;

		sequence_loader->sequence_dir = dirname(tmp);
		totalpatterns = 0;
		ini_parse(filename, sequence_loader, sequence_setter);
		free(tmp);

		int i,j;

		for(i=0; i<sequence_loader->map_count; i++) {
			if (!sequence_loader->next_map[i])
				continue;
			for(j=0;j<totalpatterns;j++) {
				if(strcmp(list[j]->name, sequence_loader->next_map[i]))
						continue;
				sequence_loader->next_index[i] = j + 1;
				break;
			}	

			free(sequence_loader->next_map[i]);
		}

		for (j=0;j<totalpatterns;j++) {
			MidiPattern *p = list[j];
			for (i=0;i<MAX_ACTIONS;i++) {
				if (p->actions[i]&0xFF) {
					int v = (p->actions[i]&0xFF) - 1;
					int vm = 0;
					if (v < sequence_loader->map_count) {
						vm = sequence_loader->next_index[v];
					}

					p->actions[i] = (p->actions[i] & 0xFF00) | vm;
				}
			}

			if (p->next) {
				int v = p->next - 1;
				int vm = 0;
				if (v < sequence_loader->map_count) {
					vm = sequence_loader->next_index[v];
				}

				p->next = vm;
			}
		}

		free(sequence_loader);
		return totalpatterns;
	}
	return 0;
}
