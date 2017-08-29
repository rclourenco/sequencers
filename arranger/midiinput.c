#include <pthread.h>
#include <alsa/asoundlib.h>
#include "seq.h"

typedef struct {
	unsigned char scmd;
	unsigned char sp1;
	int st;	
}MidiParser;

typedef struct {
	unsigned char cmd;
	unsigned char p1;
	unsigned char p2;
}MidiEvent;


void midiParserInit(MidiParser *mp)
{
	mp->st = 0;
}

int midiParser(MidiParser *mp,int input, MidiEvent *me)
{
	if( (input & 0xF0) == 0xF0) {
		me->cmd = input;
		me->p1 = 0;
		me->p2 = 0;

		return 1;
	}

	switch(mp->st) {
		case 0: if( input > 127 ) { 
				mp->scmd = input; 
				mp->st = 1; 
			} 
			break;
		case 1:
			if( input > 127) {
				 mp->scmd=input; 
				 mp->st = 1;
			}
			else{
				mp->sp1=input;
				mp->st = 2;
				if( (mp->scmd & 0xF0) == 0xC0 ) {
					mp->st = 1;
					me->cmd =  mp->scmd;
					me->p1  =  mp->sp1;
					me->p2  =  0;
					return 1;				
				}
			}
			break;
			
		case 2:
			if( input > 127 ) {
				 mp->scmd=input; 
				 mp->st = 1;
			}
			else{
				mp->st = 1;
				me->cmd =  mp->scmd;
				me->p1  =  mp->sp1;
				me->p2  =  input;
				return 1;
			}
			break;
	}
	me->cmd = 0;
	return 0;
}


int midi_init(SeqData *seqdata) 
{
	snd_rawmidi_info_t *rawmidi_info;
	
	if(seqdata->mididevice_in) {
		printf("Opening midi in %s\n",seqdata->mididevice_in);
		int err = snd_rawmidi_open(&seqdata->midihandle_in,NULL,seqdata->mididevice_in,0);
		if (err) {
		        fprintf(stderr,"snd_rawmidi_open %s failed: %d\n",seqdata->mididevice_out,err);
				return err;
		}
		else{
			seqdata->midi_in   = 1;
			if(!snd_rawmidi_info_malloc(&rawmidi_info)) {
				snd_rawmidi_info(seqdata->midihandle_in,rawmidi_info);
				const char *id   = snd_rawmidi_info_get_id(rawmidi_info);
				const char *name = snd_rawmidi_info_get_name(rawmidi_info);
				const char *subdevicename = snd_rawmidi_info_get_subdevice_name(rawmidi_info);

				int card   = snd_rawmidi_info_get_card(rawmidi_info);
				int device = snd_rawmidi_info_get_device(rawmidi_info);
				int subdevice = snd_rawmidi_info_get_subdevice(rawmidi_info);
				printf("Midi out Id: %s - %s - %s - %d - %d - %d\n", id, name,subdevicename, card,device,subdevice);
				snd_rawmidi_info_free(rawmidi_info);
			}

		}

	}
	
	if(seqdata->mididevice_out) {
    	printf("Opening midi out %s\n",seqdata->mididevice_out);
		int err = snd_rawmidi_open(NULL,&seqdata->midihandle_out,seqdata->mididevice_out,0);
		if (err) {
		        fprintf(stderr,"snd_rawmidi_open %s failed: %d\n",seqdata->mididevice_out,err);
				return err;
		}
		else{
//				snd_seq_set_client_name(seqdata->midihandle_out, "Bolos");
			seqdata->midi_out  = 1;
			if(!snd_rawmidi_info_malloc(&rawmidi_info)) {
				snd_rawmidi_info(seqdata->midihandle_out,rawmidi_info);
				const char *id   = snd_rawmidi_info_get_id(rawmidi_info);
				const char *name = snd_rawmidi_info_get_name(rawmidi_info);
				const char *subdevicename = snd_rawmidi_info_get_subdevice_name(rawmidi_info);

				int card   = snd_rawmidi_info_get_card(rawmidi_info);
				int device = snd_rawmidi_info_get_device(rawmidi_info);
				int subdevice = snd_rawmidi_info_get_subdevice(rawmidi_info);
				printf("Midi out Id: %s - %s - %s - %d - %d - %d\n", id, name,subdevicename, card,device,subdevice);
				snd_rawmidi_info_free(rawmidi_info);
			}
			
		}
	}
         
	
	return 0;
}


void midi_dispatcher(MidiEvent *me, SeqData *sd);

void *midi_input_th(void *sdv)
{
	
	SeqData *sdx = (SeqData *)sdv;
	MidiParser mp1;
	MidiEvent me1;

/*	snd_rawmidi_status_t *midi_status;

	snd_rawmidi_status_malloc(&midi_status);

	int nfds;
*/
	if(!sdx->midihandle_in) {
		return NULL;
	}

/*	struct pollfd *pfds;
	nfds = snd_rawmidi_poll_descriptors_count (sdx->midihandle_in);
	if (nfds <= 0) {
		 fprintf(stderr,"MIDI Invalid poll descriptors count\n");
		 exit(1);
	}
	pfds = malloc( sizeof(struct pollfd) * nfds );
	if (pfds == NULL) {
		     fprintf(stderr,"No enough memory\n");
		     exit(1);
	}

	int err;

	if ((err = snd_rawmidi_poll_descriptors(sdx->midihandle_in, pfds, nfds)) < 0) {
		 printf("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
		 exit(1);
	}
*/

	while(1) {

			unsigned short revents;
			unsigned char ch;
			int status;
/*			poll(pfds, nfds, -1);
/			snd_rawmidi_poll_descriptors_revents(sdx->midihandle_in, pfds, nfds, &revents);
			if(revents & POLLERR) {
				printf("Poll error\n");
			}
			size_t xrunc = snd_rawmidi_status_get_xruns(midi_status);
			if(xrunc)
				printf("XRUNC %d\n",xrunc);

			if(revents & POLLIN ){ */
				status = snd_rawmidi_read(sdx->midihandle_in,&ch,1);
			
				if(status > 0) {
				  printf("Midi in\n");
					if(midiParser(&mp1,ch,&me1)) {
						midi_dispatcher(&me1,sdx);
					}
				}
			/*}*/
	}
}


int midi_mode = 0;
void midi_dispatcher(MidiEvent *me, SeqData *sd)
{
	int control_channel = 16;
	
	if(control_channel > 0)
	{
		int v = 0x90 + control_channel - 1;
		if(me->cmd == v)
		{ 
			int key = me->p1 % 12;
			int i;
			for(i=0;i<12;i++)
				printf("=> %d \n",sd->song.state[sd->seqstatus_new.state].schange[i]);
			int nextstatus = sd->song.state[sd->seqstatus_new.state].schange[key];
			printf("NEXT: %d\n",nextstatus);

			pthread_mutex_lock(&count_mutex);
			if(key == 0 && sd->seqstatus_new.play == STOP)
				sd->seqstatus_new.play = PLAY;
			if(key == 11 )
				sd->seqstatus_new.play = STOP;
			if(nextstatus >= 0)
					sd->seqstatus_new.nextstate = nextstatus;
/*			if(group < sd->song.group_count) {*/
/*				sd->seqstatus_new.group = group;*/
/*			}*/
			pthread_mutex_unlock(&count_mutex);
		}
	}
}
