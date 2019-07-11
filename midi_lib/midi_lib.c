#include <pthread.h>
#include <alsa/asoundlib.h>
#include "midi_lib.h"

MidiDriver *midi_driver_default;

int input_mode  = 0;
int output_mode = 0;

void midi_set_input_blocking(int state)
{
	if(state==0)
	       	input_mode |= SND_RAWMIDI_NONBLOCK;
	else
		input_mode &= ~SND_RAWMIDI_NONBLOCK;
}

void midi_set_output_blocking(int state)
{
	if(state==0)
	       	output_mode |= SND_RAWMIDI_NONBLOCK;
	else
		output_mode &= ~SND_RAWMIDI_NONBLOCK;
}

int midi_init_alsa(MidiDriver **mididriver_p, const char *device_in, const char *device_out);

void midi_out_f_dummy(MidiDriver *md, const char *msg, size_t len) {
/*  fprintf(stderr, "[DEBUG] Received midi msg len %u\n", len);*/
}

int midi_port_install(const char *midi_opts)
{
  char drivertype[128];
  char device_in[128];
  char device_out[128];
  const char *driver_options;
  drivertype[0] ='\0';
  device_in[0]  ='\0';
  device_out[0] ='\0';

  if(!midi_opts) {
    // init dummy
    return 0;
  }

  int p=0;
  while(midi_opts[p] && midi_opts[p]!=';') p++;

  if( !strncmp(midi_opts,"alsa",p) ) {
    printf("Driver is Alsa\n");
    if( midi_opts[p] ) {
      p++;
      driver_options = &midi_opts[p];
      p=0;

      while(driver_options[p]) {
        while(driver_options[p] && driver_options[p]!=';') p++;

        if( (!strncmp(driver_options,"in:",3)) && p>3 ) {
          strncpy(device_in, driver_options+3, p-3);
          printf("Len: %u\n",p);
          device_in[p-3]='\0';
        }

        if( (!strncmp(driver_options,"out:",4)) && p>4 ) {
          printf("Len: %u\n",p);
          strncpy(device_out, driver_options+4, p-4);
          device_out[p-4]='\0';
        }

        printf("D O: %s %d\n", driver_options, p);

        if( driver_options[p] ) {
          p++;
          driver_options = &driver_options[p];
          p=0;
        }
      }
    }

    const char *alsa_in = NULL;
    const char *alsa_out = NULL;
    printf("DeviceIn  %s\n", device_in);
    printf("DeviceOut %s\n", device_out);

    if( device_in[0] ) alsa_in = device_in;
    if( device_out[0] ) alsa_out = device_out;

    return midi_init_alsa(&midi_driver_default, alsa_in, alsa_out);

  } else {
    strncpy(drivertype, midi_opts, p);
    drivertype[p]='\0';
    printf("Driver Type String is %s\n", drivertype);
  }

}

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


void midi_out_f_alsa(MidiDriver *md, const char *msg, size_t len) {
  if( len ) {
    int i;
    for(i=0;i<len;i++) {
      unsigned char ch = msg[i];
      snd_rawmidi_write(md->d.alsa.midihandle_out,&ch,1);
    }
  }
}

int midi_in_f_alsa(MidiDriver *md) {
    unsigned char ch;
    size_t l = snd_rawmidi_read(md->d.alsa.midihandle_in,&ch,1);
    if (l!=1)
	    return -1;
    return ch;
}

int midi_init_alsa(MidiDriver **mididriver_p, const char *device_in, const char *device_out)
{
  *mididriver_p = NULL;
  MidiDriver *mididriver = (MidiDriver *) malloc(sizeof(MidiDriver));
  if(! mididriver ) {
    fprintf(stderr, "Cannot initilize mididriver, no memory\n");
    return -1;
  }
  *mididriver_p = mididriver;
  mididriver->type = MIDI_ALSA;
  mididriver->d.alsa.midihandle_in = NULL;
  mididriver->d.alsa.midihandle_out = NULL;
  mididriver->d.alsa.mididevice_in = NULL;
  mididriver->d.alsa.mididevice_out = NULL;
  mididriver->d.alsa.midi_in = 0;
  mididriver->d.alsa.midi_out = 0;

  mididriver->midi_out_f = midi_out_f_dummy;

  if( device_in ) {
    mididriver->d.alsa.mididevice_in = strdup(device_in);
  }

  if( device_out ) {
    mididriver->d.alsa.mididevice_out = strdup(device_out);
  }



  snd_rawmidi_info_t *rawmidi_info;

  if(mididriver->d.alsa.mididevice_in) {
		printf("Opening midi in %s\n",mididriver->d.alsa.mididevice_in);
		int err = snd_rawmidi_open(&mididriver->d.alsa.midihandle_in,NULL,mididriver->d.alsa.mididevice_in,input_mode);
		if (err) {
		        fprintf(stderr,"snd_rawmidi_open %s failed: %d\n",mididriver->d.alsa.mididevice_in,err);
		    free(mididriver);
				return err;
				
		}
		else{
			mididriver->d.alsa.midi_in   = 1;
			mididriver->midi_in_f = midi_in_f_alsa;

			if(!snd_rawmidi_info_malloc(&rawmidi_info)) {
				snd_rawmidi_info(mididriver->d.alsa.midihandle_in,rawmidi_info);
				const char *id   = snd_rawmidi_info_get_id(rawmidi_info);
				const char *name = snd_rawmidi_info_get_name(rawmidi_info);
				const char *subdevicename = snd_rawmidi_info_get_subdevice_name(rawmidi_info);

				int card   = snd_rawmidi_info_get_card(rawmidi_info);
				int device = snd_rawmidi_info_get_device(rawmidi_info);
				int subdevice = snd_rawmidi_info_get_subdevice(rawmidi_info);
				printf("Midi in Id: %s - %s - %s - %d - %d - %d\n", id, name,subdevicename, card,device,subdevice);
				snd_rawmidi_info_free(rawmidi_info);
			}

		}

	}
	
	if(mididriver->d.alsa.mididevice_out) {
    	printf("Opening midi out %s\n",mididriver->d.alsa.mididevice_out);
		int err = snd_rawmidi_open(NULL,&mididriver->d.alsa.midihandle_out,mididriver->d.alsa.mididevice_out,output_mode);
		if (err) {
		        fprintf(stderr,"snd_rawmidi_open %s failed: %d\n",mididriver->d.alsa.mididevice_out,err);
				return err;
		}
		else{
//				snd_seq_set_client_name(mididriver->midihandle_out, "Bolos");
			mididriver->d.alsa.midi_out  = 1;
			mididriver->midi_out_f = midi_out_f_alsa;
			if(!snd_rawmidi_info_malloc(&rawmidi_info)) {
				snd_rawmidi_info(mididriver->d.alsa.midihandle_out,rawmidi_info);
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


#ifdef make_test
main (int argc, char **argv)
{
  printf("Ok\n");
  if( argc > 1) {
    midi_port_install(argv[1]);
    int x = midi_in();
    printf("X: %02X\n", x);
  } else {
    midi_port_install(NULL);
  }
  
}
#endif
