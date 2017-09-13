#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <unistd.h>
#include <time.h>
/*#include <allegro.h>*/
#include <getopt.h>

#include "seq.h"
#include "seqgui.h"

#define NUM_THREADS  2
#define TCOUNT 10
#define COUNT_LIMIT 12

int     count = 0;
int     thread_ids[3] = {0,1,2};
pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;
int shutdown = 0;


SeqNote note_array[] = {
	{33,100,1,0},
	{33,100,0,0},
	{31,100,0,0},
	{28,100,0,0},
};

size_t note_array_size = 4;

SeqNote note_array_2[] = {
	{57,100,1,0},
	{57,100,0,0},
	{57,100,0,0},
	{57,100,0,0},
};

size_t note_array_size_2 = 4;

SeqNote note_array_3[] = {
	{61,100,1,0},
	{61,100,0,0},
	{61,100,0,0},
	{61,100,0,0},
};

size_t note_array_size_3 = 4;

SeqNote note_array_4[] = {
	{64,100,1,0},
	{64,100,0,0},
	{64,100,0,0},
	{64,100,0,0},
};

size_t note_array_size_4 = 4;


SeqData sdata;

void *inc_count(void *t) 
{
  int i;
  long my_id = (long)t;

  for (i=0; i<TCOUNT; i++) {
    pthread_mutex_lock(&count_mutex);
    count++;

    /* 
    Check the value of count and signal waiting thread when condition is
    reached.  Note that this occurs while mutex is locked. 
    */
    if (count == COUNT_LIMIT) {
      pthread_cond_signal(&count_threshold_cv);
      printf("inc_count(): thread %ld, count = %d  Threshold reached.\n", 
             my_id, count);
      }
    printf("inc_count(): thread %ld, count = %d, unlocking mutex\n", 
	   my_id, count);
    pthread_mutex_unlock(&count_mutex);

    /* Do some "work" so threads can alternate on mutex lock */
    sleep(1);
    }
  pthread_exit(NULL);
}

void *watch_count(void *t) 
{
  long my_id = (long)t;

  printf("Starting watch_count(): thread %ld\n", my_id);

  /*
  Lock mutex and wait for signal.  Note that the pthread_cond_wait 
  routine will automatically and atomically unlock mutex while it waits. 
  Also, note that if COUNT_LIMIT is reached before this routine is run by
  the waiting thread, the loop will be skipped to prevent pthread_cond_wait
  from never returning. 
  */
  pthread_mutex_lock(&count_mutex);
  while (count<COUNT_LIMIT) {
    pthread_cond_wait(&count_threshold_cv, &count_mutex);
    printf("watch_count(): thread %ld Condition signal received.\n", my_id);
    count += 125;
    printf("watch_count(): thread %ld count now = %d.\n", my_id, count);
    }
  pthread_mutex_unlock(&count_mutex);
  pthread_exit(NULL);
}

int note_translate(int note, int base, int key, int octave)
{
	key  %= 12;
	base %= 12;


	if( key != -1){
		note = note - base + key;
	}

	note = note+octave*12;

	if(note < 0)
		note = 12 + note;

	return note%128;
	
}

void note_off(SeqData *sdx, int voice)
{
	unsigned char ch;

	if(sdx->notes_playing[voice].d < 1)
		return;

	int channel = sdx->notes_playing[voice].d - 1;
	int note = sdx->notes_playing[voice].k % 128;

	ch=0x80+channel;
	snd_rawmidi_write(sdx->midihandle_out,&ch,1);

	ch=note;
	snd_rawmidi_write(sdx->midihandle_out,&ch,1);

	ch=0; 
	snd_rawmidi_write(sdx->midihandle_out,&ch,1);

	snd_rawmidi_drain(sdx->midihandle_out); 

	sdx->notes_playing[voice].d = 0;
	sdx->notes_playing[voice].l = 0;
}

void note_on(SeqData *sdx, int voice, int channel, int note, int vel, int legato)
{
		unsigned char ch;
		ch=0x90+channel;
		snd_rawmidi_write(sdx->midihandle_out,&ch,1);

		ch=note;
		snd_rawmidi_write(sdx->midihandle_out,&ch,1);
		//ch=sg->seq[vc].cur_note.v;
		ch = 100;
		snd_rawmidi_write(sdx->midihandle_out,&ch,1);
		snd_rawmidi_drain(sdx->midihandle_out);

		sdx->notes_playing[voice].k = note;
		sdx->notes_playing[voice].d = channel + 1;
		sdx->notes_playing[voice].v = vel;
		sdx->notes_playing[voice].l = legato;
	
}


void set_next_state(SeqData *sdx)
{
	int to_stop = 0;
	int st = sdx->seqstatus_current.state;

	if( sdx->seqstatus_current.nextstate != -1) {
		printf("Forced to %d\n",sdx->seqstatus_current.nextstate);
		sdx->seqstatus_current.state = sdx->seqstatus_current.nextstate;
		sdx->seqstatus_current.nextstate = -1;
	}
	else{
		st = sdx->song.state[st].next;
		printf("Newstate: %d\n",st);
		if( st >=0 ) {
			sdx->seqstatus_current.state = st;
		}
		else{
			to_stop = 1;
		}
	}

	int group = sdx->song.state[ sdx->seqstatus_current.state ].seq_id;

	sdx->seqstatus_current.group = group;

	pthread_mutex_lock(&count_mutex);
	if(to_stop)
		sdx->seqstatus_new.play = STOP;
	sdx->seqstatus_new.state = sdx->seqstatus_current.state;
	sdx->seqstatus_new.group = sdx->seqstatus_current.group;
	sdx->seqstatus_new.nextstate = sdx->seqstatus_current.nextstate;
	pthread_mutex_unlock(&count_mutex);

}


void *play_th(void *sdv)
{
	SeqData *sdx = (SeqData *)sdv;
	struct timespec tp;
	unsigned long usec_el, o_usec_el;
	unsigned long adiff;
	int tick = 0;

	clock_gettime(CLOCK_REALTIME, &tp);
	o_usec_el = tp.tv_nsec/1000 + (tp.tv_sec%1000) * 1000000;


	while(no_shutdown()) {
		//printf("Play\n");
		unsigned char ch;
		unsigned char note;
		clock_gettime(CLOCK_REALTIME, &tp);
		usec_el = tp.tv_nsec/1000 + (tp.tv_sec%1000) * 1000000;
		if(usec_el < o_usec_el)
			adiff = abs(1000000000+usec_el - o_usec_el);
		else
			adiff = abs(usec_el - o_usec_el);

		SeqGroup *sg = &sdx->song.group[ sdx->seqstatus_current.group ];
		

		int bpm   = sg->bpm;
		int div   = sg->div;
		int pulse = sg->pulse;

		int tick_len = ( 1000 / ( (bpm * div * pulse)/60 ) ) * 1000;

		pthread_mutex_lock(&count_mutex);
		SeqStatus ncopy = sdx->seqstatus_new;
		pthread_mutex_unlock(&count_mutex);
		
		sdx->seqstatus_current.nextstate = ncopy.nextstate;
		sdx->seqstatus_current.state = ncopy.state;
		sdx->seqstatus_current.group = ncopy.group;

		if(ncopy.play != sdx->seqstatus_current.play ) {
			sdx->seqstatus_current.play = ncopy.play;
			sg->note_p = 0;
			tick = 0;
			if(ncopy.play == PLAY) {
				pthread_mutex_lock(&count_mutex);
				sdx->seqstatus_new.nextstate = 0;
				sdx->seqstatus_current.nextstate = 0;
				pthread_mutex_unlock(&count_mutex);
				set_next_state(sdx);
					sg = &sdx->song.group[ sdx->seqstatus_current.group ];
					sg->note_p = 0;

				o_usec_el = usec_el;
				adiff = 0;
				tick = 0;
				continue;
			}
			if(ncopy.play == STOP) {
				int vc;
				for(vc=0; vc < MAXSEQS; vc++ ) {
					if(sdx->notes_playing[vc].d) {
						note_off(sdx,vc);
					}
				}
			}
		}
		if(ncopy.key != sdx->seqstatus_current.key) {
			if( tick==0 ) {
				sdx->seqstatus_current.key = ncopy.key;
			}
		}

		if(ncopy.group != sdx->seqstatus_current.group) {
			if( tick==0 ) {
				int cpos = sg->note_p;

				if( cpos % sg->frag < sg->frag / 2  ) {
					sdx->seqstatus_current.group = ncopy.group;
					sg = &sdx->song.group[ sdx->seqstatus_current.group ];
					sg->note_p =cpos % sg->frag;
					printf("Group: %u\n",sdx->seqstatus_current.group);
					printf("Len: %u Pos:   %u\n",sg->notes_n,sg->note_p);
					printf("Voices: %u\n",sg->seq_count);
				}

			}
		}

		if(sdx->seqstatus_current.play == STOP) {
			usleep(5000);
			continue;
		}

		
		if(adiff >= tick_len) {
//			printf("Sec   %lu\n",tp.tv_sec);
//			printf("Nano  %lu\n",tp.tv_nsec);
//			printf("Micro %lu\n",usec_el); 
//			printf("Diff %lu %u\n",adiff, tick_len); 
			if(adiff - tick_len > usec_el)
				o_usec_el = 1000000000 + usec_el - (adiff - tick_len);
			else
				o_usec_el = usec_el - (adiff - tick_len);
			
			if(tick == 0) {
				
				int vc = 0;
				for(vc=0; vc < MAXSEQS; vc++ ) {
					if( vc < sg->seq_count ) {
						int channel = sg->seq[vc].channel;
						char skip = 0;
						int pos = sg->note_p % sg->seq[vc].notes_n;

						int next_note = sg->seq[vc].note_a[ pos ].k;
						int legato    = sg->seq[vc].note_a[ pos ].l;
						int vel       = sg->seq[vc].note_a[ pos ].v;

						next_note = note_translate(next_note,sg->seq[vc].base_note,sdx->seqstatus_current.key,sg->seq[vc].octave);

						if(sdx->notes_playing[vc].d && sdx->notes_playing[vc].l ) {

							if(sdx->notes_playing[vc].k != next_note ) {
								note_off(sdx,vc);
							}
							else {
								skip = 1;
								sdx->notes_playing[vc].l = legato;
							}
						}

						if(!skip) {

							note_on(sdx,vc,channel,next_note, vel,legato);
	//						printf("Note on %d\n", sd->voices[vc].cur_note.k );
						}

					}
					else{
						if(sdx->notes_playing[vc].d) {
							note_off(sdx,vc);
						}
					}
				}
			}
			
			if(tick == pulse - 1 ) {
				int vc = 0;
				for(vc=0; vc < MAXSEQS; vc++ ) {
					if( sdx->notes_playing[vc].d && !sdx->notes_playing[vc].l) {
						note_off(sdx,vc);
					}

				}
	
				if( sg->note_p +1 == sg->notes_n ) {
					printf("DEBUG %d %d\n",sg->note_p, sg->notes_n);
					set_next_state(sdx);
					sg = &sdx->song.group[ sdx->seqstatus_current.group ];
					sg->note_p = 0;
				}
				else
					sg->note_p = (sg->note_p +1 ) % sg->notes_n;
			}

			tick = (tick + 1) % pulse;
		}
		else {
			if(tick_len>adiff){
				usleep(tick_len - adiff);
			}
		}
	}
	printf("Playing thread goes off...\n");
	pthread_exit(NULL);
}



SeqSong song;

int start_group(SeqSong *song, int group)
{
	if(group <0 || group > song->group_count) {
		return 0;
	}
	
	SeqGroup *sg = &song->group[group];
	SeqVoice *voices = sg->seq;
	int nvoices      = sg->seq_count;

	song->cur_group = group;
	sg->note_p = 0;

}


#define MAXSYNCON 4

typedef struct {
	char *song_file;
	char *song_dir;
	char *midi_in;
	char *midi_out;
	char *synth2connect[MAXSYNCON];
}RunOptions;

int load_options(int argc, char *argv[], RunOptions *options)
{
	int c;
	int i;
    int digit_optind = 0;
	options->midi_in   = NULL;
	options->midi_out  = NULL;
	options->song_file = NULL;
	options->song_dir = NULL;
	int synth_count = 0;
	for(i=0;i<MAXSYNCON;i++)
		options->synth2connect[i] = NULL;

   while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"mdin",     required_argument, 0,  0 },
            {"mdout",    required_argument, 0,  0 },
            {"song",  required_argument,  0,  0 },
            {"dir",   required_argument,  0,  0 },
            {"synth",  required_argument, 0, 0},
            {0,         0,                 0,  0 }
        };

       c = getopt_long(argc, argv, "i:o:s:d:y:",
                 long_options, &option_index);
        if (c == -1)
            break;

       switch (c) {
		    case 0:
				switch(option_index)
				{
					case 0: if(options->midi_in == NULL)
								options->midi_in = strdup(optarg);
							break;
					case 1: if(options->midi_out == NULL)
								options->midi_out = strdup(optarg);
							break;
					case 2: if(options->song_file == NULL)
								options->song_file = strdup(optarg);
							break;
					case 3: if(options->song_dir == NULL)
								options->song_dir = strdup(optarg);
							break;
					case 4:
							if(synth_count < MAXSYNCON) {
								if(options->synth2connect[synth_count] == NULL)
									options->synth2connect[synth_count++] = strdup(optarg);
							}
						break;
				}
		        break;

		   case 'i':
					if(options->midi_in == NULL)
						options->midi_in = strdup(optarg);
		        break;

		   case 'o':
					if(options->midi_out == NULL)
						options->midi_out = strdup(optarg);
		        break;

		   case 's':
					if(options->song_file == NULL)
						options->song_file = strdup(optarg);
		        break;

		   case 'd':
					if(options->song_dir == NULL)
						options->song_dir = strdup(optarg);
		        break;

			case 'y':
					if(synth_count < MAXSYNCON) {
						if(options->synth2connect[synth_count] == NULL)
							options->synth2connect[synth_count++] = strdup(optarg);
					}
			break;
		   case '?':
		        break;

		   default:
		        printf("?? getopt returned character code 0%o ??\n", c);
		    }
    }

	if(!options->midi_in) {
		fprintf(stderr,"Midi In is missing...\n");
		return 0;
	}

	if(!options->midi_out) {
		fprintf(stderr,"Midi Out is missing...\n");
		return 0;
	}

	if(!options->song_file && !options->song_dir) {
		fprintf(stderr,"Song file/dir is missing\n");
		return 0;
	}

	 return 1;
}

RunOptions roptions;

char *progdir(char *progpath)
{
	char *dir = strdup(progpath);
	if(!dir)
		return NULL;

	char *pos = strrchr(dir, '/');
	*pos = '\0';
	return dir;
}

char cmdline_buffer[4096];
void connect_midi(char *dir,char *devices[])
{
	if(!devices[0])
		return;
	if(*dir) {
		sprintf(cmdline_buffer,"perl %s/connectdevices.pl",dir);
	}
	else{
		strcpy(cmdline_buffer,"perl connectdevices.pl");
	}

	int i;
	for(i=0;i<MAXSYNCON;i++){
		if(devices[i]) {
			strcat(cmdline_buffer," \"");
			strcat(cmdline_buffer,devices[i]);
			strcat(cmdline_buffer,"\"");
		}
	}
	
	system(cmdline_buffer);
}


int main (int argc, char *argv[])
{
  int i, rc;
  long t1=1, t2=2, t3=3;


	if(!load_options(argc, argv, &roptions)) {
		fprintf(stderr,"Abort...\n");
		return 1;
	}

  pthread_t threads[3];
  pthread_attr_t attr;

	struct timespec tres;
	struct timespec tp;

	sdata.seqstatus_current.nextstate = -1;
	sdata.seqstatus_new.nextstate = -1;
	sdata.seqstatus_current.state = 0;
	sdata.seqstatus_new.state     = 0;

	sdata.seqstatus_current.play = STOP;
	sdata.seqstatus_new.play     = STOP;
	sdata.seqstatus_current.key = -1;
	sdata.seqstatus_new.key     = -1;

	

	printf("Progname: %s\n", argv[0]);


	clock_getres(CLOCK_REALTIME, &tres);
	printf("Sec  res %lu\n",tres.tv_sec);
	printf("Nano res %lu\n",tres.tv_nsec);

	clock_gettime(CLOCK_REALTIME, &tp);
	printf("Sec   %lu\n",tp.tv_sec);
	printf("Nano  %lu\n",tp.tv_nsec);


	parse_seq(roptions.song_file,&sdata.song);

//	dump_song(&sdata.song);
	printf("States: %d\n", sdata.song.states_alloced);

	int group = sdata.song.state[sdata.seqstatus_current.state].seq_id;
	printf("Start at: %d\n",group);
	sdata.seqstatus_current.group = group;
	sdata.seqstatus_new.group     = group;


	for(i=0;i<MAXSEQS;i++) {
		sdata.notes_playing[i].d = 0;
	}


	sdata.midihandle_out = 0;
	sdata.midihandle_in  = 0;
	sdata.mididevice_in  = roptions.midi_in;
	sdata.mididevice_out = roptions.midi_out;
	midi_init(&sdata);

	char *dir = progdir(argv[0]);
	connect_midi(dir,roptions.synth2connect);
	if(dir)
		free(dir);
/*
	Prepare devices
	
*/

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init(&count_mutex, NULL);
  pthread_cond_init (&count_threshold_cv, NULL);

  /* For portability, explicitly create threads in a joinable state */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  AllegroSet aset;
  if(!init_allegro(&aset)) {
    fprintf(stderr, "Error: Cannot init allegro\n");
    exit(2);
  }
  init_gui(&aset);

  pthread_create(&threads[0], &attr, play_th, (void *)&sdata);
  pthread_create(&threads[1], &attr, midi_input_th, (void *)&sdata);
//  pthread_create(&threads[2], &attr, inc_count, (void *)t3);

	run_dialog();
	shutdown_allegro(&aset);
	//return 0;
	int inp;

	printf("After get char...\n");
/*	while(inp=readkey() ) {
		inp &= 0xff;
		if(inp >= '0' && inp <= '9') {
			pthread_mutex_lock(&count_mutex);
			sdata.seqstatus_new.group = (inp - '0') % sdata.song.group_count;
			pthread_mutex_unlock(&count_mutex);
		}
	}
*/
  //
  /* Wait for all threads to complete */
  pthread_mutex_lock(&count_mutex);
  shutdown = 1;
  pthread_mutex_unlock(&count_mutex);
  for (i=0; i<NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
  printf ("Main(): Waited on %d  threads. Done.\n", NUM_THREADS);

  /* Clean up and exit */
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&count_mutex);
  pthread_cond_destroy(&count_threshold_cv);
/*  pthread_exit(NULL);*/

  printf("Bye...\n");
	return 0;
}

int no_shutdown()
{
  int a;
  pthread_mutex_lock(&count_mutex);
  a = !shutdown;
  pthread_mutex_unlock(&count_mutex);
  return a;
}
