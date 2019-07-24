#ifndef _MIDI_PATTERN
  #define _MIDI_PATTERN
typedef struct {
  unsigned char type[4];
  unsigned char length[4];
} MidiFilePrefix;

typedef struct {
  char type[5];
  size_t length;
} MidiPrefix;

typedef struct {
  unsigned char format[2];
  unsigned char tracks[2];
  unsigned char division[2];
} MidiFileHeader;

typedef struct {
  unsigned int format;
  unsigned int tracks;
  unsigned int ticks_quarter;
  unsigned int frames_second;
  unsigned int ticks_frame;
} MidiHeader;


typedef enum {meCV, meCVRunning, meCommon, meSystem, meSysex, meMeta} MidiEventType;

typedef struct _MidiEventNode {
  struct _MidiEventNode *next;
  unsigned long time;
  MidiEventType type;
  union {
    struct {
      unsigned char type;
      unsigned char channel;
    } vc; /*voice channel*/
    struct {
      unsigned char type;
    } common;
    struct {
      unsigned char type;
    } system;
    struct {
      unsigned char type;
      unsigned long length;
    } sysex;
    struct {
      unsigned char type;
      unsigned long length;
    } meta;
  } t;
  size_t datalen;
  unsigned char data[0];
} MidiEventNode;

typedef struct _MidiTrackNode {
  struct _MidiTrackNode *next;
  MidiEventNode *events;
  MidiEventNode *playing;
} MidiTrackNode;

#define PATTERN_MAXFILENAME 255
typedef struct _MidiPattern {
  char filename[PATTERN_MAXFILENAME+1];
  MidiTrackNode *tracks;
  unsigned int ticks_quarter;
  unsigned long tempo;
//  unsigned long bpm;
//  unsigned char meter_a;
//  unsigned char meter_b;
  /*TODO*/
} MidiPattern;

typedef struct {
  enum {StatusDelta, StatusCmd, StatusData, StatusSysex, StatusMeta, StatusError, StatusSysexData, StatusMetaData, StatusMetaLen} status;
  unsigned char ch; /*current input*/
  unsigned char cmd;
  unsigned long delta;
  unsigned long orgdatalen;
  unsigned long datalen;
  unsigned long eolen;
  unsigned long elen;
  unsigned char metatype;
  unsigned char channel;
  unsigned long time;
  MidiEventNode *first;
  MidiEventNode *current;
} TrackParser;

int parse_track(TrackParser *tp);
MidiPattern *midi_pattern_load(char *filename);
void midi_pattern_free(MidiPattern **pat_r);
#endif
