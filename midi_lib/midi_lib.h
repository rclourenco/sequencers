#ifndef _MIDI_LIB

#define _MIDI_LIB
#include <alsa/asoundlib.h>

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

void midiParserInit(MidiParser *mp);
int midiParser(MidiParser *mp,int input, MidiEvent *me);

typedef enum {MIDI_NONE, MIDI_ALSA} MidiDriverType;

typedef struct {
  int initialized;
} MidiDriverNone;

typedef struct {
  int ok;
  snd_rawmidi_t *midihandle_in;
  snd_rawmidi_t *midihandle_out;
  char *mididevice_in;
  char *mididevice_out;
  int midi_in;
  int midi_out;
} MidiDriverAlsa;


typedef struct _midi_driver {
  MidiDriverType type;
  union {
    MidiDriverNone none;
    MidiDriverAlsa alsa;
  } d;
  void (*midi_out_f)(struct _midi_driver *md, const char *msg, size_t len);
  int  (*midi_in_f)(struct _midi_driver *md);
}MidiDriver;

extern MidiDriver *midi_driver_default;

#define midi_out(m, l) if( midi_driver_default ) \
  midi_driver_default->midi_out_f(midi_driver_default, (m), (l))
#define midi_in() (( midi_driver_default ) ? midi_driver_default->midi_in_f(midi_driver_default) : -1)

int midi_port_install(const char *midi_opts);

void midi_set_input_blocking(int state);
void midi_set_output_blocking(int state);

#endif
