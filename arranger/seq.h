#ifndef SEQ_H
	#define SEQ_H
#define MAXPARSELINE 2048

#include <alsa/asoundlib.h>

typedef struct {
	char k; /*valor*/
	unsigned char v; /*velocity*/
	unsigned char l; /*legato*/
	unsigned char d; /*reserved*/
}SeqNote;

#define MAXCRL 256

typedef struct {
	SeqNote *note_a;
	SeqNote cur_note;
	char keys[12];
	int  key_filtering;
	int  notes_n;
	int  size;
	int last_played;

	int channel;
	int base_note;
	int octave;
	int velocity;
}SeqVoice;




typedef struct {
	int octave;
	int channel;
	int rhythm_len;
	char rhythm[MAXCRL];
	char keys[12][4];
	int  note_p;
}SeqChord;

#define MAXSEQS 8
#define MAXCHORDS 4

typedef struct {
	int bpm;
	int div;
	int pulse;
	int frag;

	int seq_count;
	int chord_count;

	SeqVoice seq[MAXSEQS];
	SeqChord chord[MAXCHORDS];

	int  notes_n;
	int  note_p;

	/*triggers*/
	int trigger_key;
	int trigger_prg;
}SeqGroup;

#define MAXGROUPS 10

typedef struct {
	int seq_id;
	int next;
	int schange[12];
	int keymap[12];

	/*
		TODO
		trigger midi on enter
	*/

}SeqState;

typedef struct {
	char song_name[256];

	int group_count;
	int groups_alloced;
	int state_count;
	int states_alloced;

	SeqGroup *group;
	SeqState *state;

	/*play data*/
	int cur_state;
	int cur_group;
}SeqSong;

#define STOP 0
#define PLAY 1

typedef struct {
	char key;
	int group;
	int play;
	int state;
	int nextstate;
}SeqStatus;

typedef struct {
	snd_rawmidi_t *midihandle_in;
	snd_rawmidi_t *midihandle_out;

	snd_seq_t  *seqhandle_in;


	SeqNote notes_playing[MAXSEQS];
	int nvoices;

	char *mididevice_in;
	char *mididevice_out;
	int midi_in;
	int midi_out;

	SeqSong song;
	int  note_p;
	SeqNote cur_note;

	int bpm;
	int div;
	int pulse;

	SeqStatus seqstatus_new;
	SeqStatus seqstatus_current;
}SeqData;


#include <pthread.h>
extern pthread_mutex_t count_mutex;

/*midiinput.c*/
int midi_init(SeqData *seqdata);
void *midi_input_th(void *sdv);

/*parse_seq.c*/
int parse_seq(char *filename, SeqSong *ss);

extern SeqData sdata;
#endif
