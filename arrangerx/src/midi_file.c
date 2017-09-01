#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


typedef enum {meNoteOn, meNoteOff, meAny, meSysex, meMeta} MidiEventType;

typedef struct _MidiEventNode {
  struct _MidiEventNode *next;
  MidiEventType type;
  unsigned char metatype;
  size_t datalen;
  unsigned char firstdata;
} MidiEventNode;


typedef struct {
  char *pointer;
  size_t limit;
  size_t current;
} MemPool;

#define POOL_LIMIT 1024*1024
char buffer[POOL_LIMIT];

char *init_mem_pool(MemPool *pool, void *buffer, size_t limit)
{
  pool->limit = limit;
  pool->current = 0;

  if(buffer) {
    pool->pointer = buffer;
  }
  else {
    pool->pointer = (char *)malloc(limit);
  }

  return pool->pointer;
}


void reset_mem(MemPool *pool)
{
  pool->current = 0;
}

void *alloc_mem(MemPool *pool, size_t len)
{
  if( pointer && pool->current + len < pool->limit ) {
    pool->current += len;
    return &pointer[pool->current];
  }
  return NULL;
}

int store_event_data( TrackParser *tp, unsigned char data )
{
  if( tp->en.datalen == 0 ) {
    tp->en.firstdata = data;
    tp->en.datalen++;
    return 1;
  }
  else {
    char *datap;
    if( datap = (char *)alloc_mem(tp->mempool, 1) ) {
      *datap = data;
      tp->en.datalen++;
      return 1;
    }
  }
  return 0; /*Error*/
}

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
} TrackParser;

const char *metalabel(unsigned char metatype) {
  const char *st = "Meta Unknown";
  static char buffer[20];
  switch(metatype)
  {
    case 0: st = "Sequence Number"; break;
    case 1: st = "Text Event"; break;
    case 2: st = "Copyright Notice"; break;
    case 3: st = "Sequence/Track Name"; break;
    case 4: st = "Instrument Name"; break;
    case 5: st = "Lyric"; break;
    case 6: st = "Marker"; break;
    case 7: st = "Cue Point"; break;
    case 0x20: st = "Midi Channel Prefix"; break;
    case 0x2F: st = "End of Track"; break;
    case 0x51: st = "Set Tempo"; break;
    case 0x54: st = "SMTPE Offset"; break;
    case 0x58: st = "Time Signature"; break;
    case 0x59: st = "Key Signature"; break;
    case 0x7F: st = "Sequencer-Specific Meta-event"; break;
    default:
      sprintf(buffer,"Meta[%02X]", metatype);
      st = buffer;
  }
  return st;
}

void show_meta_data(unsigned char metatype, unsigned char byte) {
  switch( metatype ) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    if( byte >= 32 && byte < 127 ) {
      putchar(byte);
    }
    else {
      printf("<%02X>", byte);
    }
    break;
    default: printf("%02X ", byte); break;
  }
}

const char *midi_cmd_string(unsigned char cmd, char running) {
  const char *st = running ? "<Unknown>" : "Unknown";
  switch( cmd ) {
    case 1: st = running ? "<Note Off>"          : "Note Off";          break;
    case 2: st = running ? "<Note On>"           : "Note On";           break;
    case 3: st = running ? "<Poly Key Pressure>" : "Poly Key Pressure"; break;
    case 4: st = running ? "<Controller Change>" : "Controller Change"; break;
    case 5: st = running ? "<Program Change>"    : "Program Change";    break;
    case 6: st = running ? "<Channel Pressure>"  : "Channel Pressure";  break;
    case 7: st = running ? "<Pitch Bend>"        : "Pitch Bend";        break;
  }
  return st;
}

int read_prefix(FILE *fp, MidiPrefix *prefix)
{
  MidiFilePrefix mfp;
  if( fread(&mfp, sizeof(MidiFilePrefix), 1, fp) == 1 ) {
    prefix->length = (mfp.length[0]<<24) + (mfp.length[1]<<16) + (mfp.length[2]<<8) + mfp.length[3];
    strncpy(prefix->type,mfp.type,4);
    prefix->type[4]='\0';
    return 1;
  }
  return 0;
}

int read_header(FILE *fp, MidiHeader *header) {
  MidiFileHeader mfh;
  if( fread(&mfh, sizeof(MidiFileHeader), 1, fp) == 1 ) {
    header->format = (mfh.format[0] << 8) + mfh.format[1];
    header->tracks = (mfh.tracks[0] << 8) + mfh.tracks[1];
    return 1;
  }
  return 0;
}


void init_track_parser(TrackParser *tp) {
  tp->status = StatusDelta;
  tp->delta  = 0L;
  tp->cmd    = 0;
  tp->time = 0L;
}

/*
  return !0 if overflow
*/
int update_track_time(TrackParser *tp)
{
  if( tp->time + tp->delta < tp->time ) {
    tp->time = -1;
    return -1;
  }
  tp->time += tp->delta;
  return 0;
}

void parse_new_delta(TrackParser *tp)
{
  tp->status = StatusDelta;
  tp->delta  = 0L;
}

int read_track(FILE *fp, size_t len) {
  int i;
  TrackParser tp;
  init_track_parser(&tp);
  printf("Track Data:\n");
  for(i=0;i<len;i++) {
    int ch = fgetc(fp);
    if(ch==EOF)
      break;
    tp.ch = ch;
    parse_track(&tp);
  }
  printf("\n**********************************\n\n");
}


int parse_track(TrackParser *tp) {
  if( tp->status == StatusError )
    return 2;

  switch(tp->status) {
    case StatusDelta:
        tp->delta = (tp->delta * 128) + (tp->ch & 0x7F);
        if( (tp->ch & 0x80) == 0 ) {
          tp->status = StatusCmd;
          update_track_time(tp);
        }
    break;
    case StatusCmd:
      if( (tp->ch & 0x80) != 0 ) {
        tp->orgdatalen = 0;
        switch(tp->ch & 0xF0) {
          case 0x80: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 1; tp->channel = tp->ch & 0x0F; break;
          case 0x90: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 2; tp->channel = tp->ch & 0x0F; break;
          case 0xA0: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 3; tp->channel = tp->ch & 0x0F; break;
          case 0xB0: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 4; tp->channel = tp->ch & 0x0F; break;
          case 0xC0: tp->orgdatalen = 1; tp->status = StatusData; tp->cmd = 5; tp->channel = tp->ch & 0x0F; break;
          case 0xD0: tp->orgdatalen = 1; tp->status = StatusData; tp->cmd = 6; tp->channel = tp->ch & 0x0F; break;
          case 0xE0: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 7; tp->channel = tp->ch & 0x0F; break;
          case 0xF0:
            switch( tp->ch ) {
              case 0xF0: tp->status = StatusSysex; tp->cmd = 0; break;
              case 0xF1: tp->cmd = 0; parse_new_delta(tp); break;
              case 0xF2: /*Song Position Pointer*/ tp->status = StatusData; tp->orgdatalen = 2; tp->cmd = 0; break;
              case 0xF3: /*Song Select*/ tp->status = StatusData; tp->orgdatalen = 1; tp->cmd = 0; break;
              case 0xF4: /**/ tp->cmd = 0; parse_new_delta(tp); break;
              case 0xF5: /**/ tp->cmd = 0; parse_new_delta(tp); break;
              case 0xF6: /*Tune Request*/ tp->cmd = 0; parse_new_delta(tp); break;
              case 0xF7: tp->status = StatusSysex; tp->cmd = 0; break;
              case 0xF8: /*Timing Clock*/ tp->orgdatalen = 0; parse_new_delta(tp); break;
              case 0xF9: /*Unknown*/      tp->orgdatalen = 0; parse_new_delta(tp); break;
              case 0xFA: /*Start*/        tp->orgdatalen = 0; parse_new_delta(tp); break;
              case 0xFB: /*Stop*/         tp->orgdatalen = 0; parse_new_delta(tp); break;
              case 0xFC: /*Unknown*/      tp->orgdatalen = 0; parse_new_delta(tp); break;
              case 0xFD: /*Unknown*/      tp->orgdatalen = 0; parse_new_delta(tp); break;
              case 0xFE: /*Active Sensing*/ tp->orgdatalen = 0; parse_new_delta(tp); break;
              case 0xFF: tp->status = StatusMeta; tp->cmd = 0; break;
              default: tp->cmd = 0; tp->orgdatalen = 0; parse_new_delta(tp);
            }
          break;
          default:
            tp->status = StatusError;
        }
        tp->elen = 0;
        if( tp->status != StatusError && tp->ch < 0xF0 ) {
            printf("@%010lu Channel %u %s (%lu) [ ", tp->time, tp->channel+1, midi_cmd_string(tp->cmd, 0), tp->orgdatalen);
        }
        tp->datalen = tp->orgdatalen;
      }
      else {
        if( tp->orgdatalen == 0 || !tp->cmd ) {
            /* Ignore these data bytes*/
            /*tp->status = StatusError;*/
        }
        else {
          tp->datalen = tp->orgdatalen;
          printf("@%010lu Channel %u %s (%lu) [ %02X ", tp->time, tp->channel+1, midi_cmd_string(tp->cmd, 1), tp->orgdatalen, tp->ch );
          tp->datalen--;
          if(tp->datalen == 0) {
            printf("]\n");
            parse_new_delta(tp);
          }
          else {
            tp->status = StatusData;
          }
        }
      }
    break;
    case StatusData:
      if( (tp->ch & 0x80) || tp->datalen == 0) {
        tp->status = StatusError;
      }
      else {
        printf("%02X ", tp->ch);
        tp->datalen--;
        if(tp->datalen == 0) {
          printf("]\n");
          parse_new_delta(tp);
        }
      }
    break;
    case StatusSysex:
      tp->elen = (tp->elen * 128) + (tp->ch & 0x7F);
      if( (tp->ch & 0x80) == 0 ) {
        tp->status = StatusSysexData;
        if( tp->elen > 0 ) {
          tp->status = StatusSysexData;
          printf("@%010lu SysexData (%lu) [ ", tp->time, tp->elen);
        }
        else {
          printf("@%010lu SysexData (%lu) [ ]", tp->time, tp->elen);
          parse_new_delta(tp);
        }
      }
    break;
    case StatusMeta:
      tp->metatype = tp->ch;
      tp->status = StatusMetaLen;
      tp->elen   = 0;
    break;
    case StatusMetaLen:
      tp->elen = (tp->elen * 128) + (tp->ch & 0x7F);
      if( (tp->ch & 0x80) == 0 ) {
        if( tp->elen > 0 ) {
          tp->status = StatusMetaData;
          printf("@%010lu %s (%lu) [ ", tp->time, metalabel(tp->metatype), tp->elen);
        }
        else {
          printf("@%010lu %s", tp->time, metalabel(tp->metatype));
          parse_new_delta(tp);
        }
      }
    break;
    case StatusSysexData:
      if( tp->elen == 0) {
        printf("] *** Error ***\n");
        tp->status = StatusError;
      }
      else {
        printf("%02X ", tp->ch );
        tp->elen--;
        if(tp->elen == 0) {
          printf("]\n");
          parse_new_delta(tp);
        }
      }
    break;
    case StatusMetaData:
      if( tp->elen == 0) {
        printf("] *** Error ***\n");
        tp->status = StatusError;
      }
      else {
        show_meta_data(tp->metatype, tp->ch);
        tp->elen--;
        if(tp->elen == 0) {
          printf("]\n");
          parse_new_delta(tp);
        }
      }
    break;
  }
}


main(int argc, char **argv)
{
  FILE *fp;
  if(argc > 1) {
    fp = fopen(argv[1],"rb");
    if(!fp) {
      fprintf(stderr,"Cannot open file %s\n", argv[1]);
      return 2;
    }
  }

  MidiPrefix mp;
  MidiHeader mh;
  while( read_prefix(fp,&mp) ) {
    printf("Type %s Len: %u\n", mp.type, (unsigned int)mp.length);
    long offset = mp.length;
    if( !strcmp(mp.type,"MThd") && mp.length == 6 ) {
      if(!read_header(fp, &mh)) {
        break;
      }
      printf("Format %u Tracks %u\n", mh.format, mh.tracks );
    }
    else if( !strcmp(mp.type,"MTrk") ) {
      read_track(fp, mp.length);
    }
    else {
      if(offset < 0)
        break;
      if(fseek(fp,offset,SEEK_CUR))
        break;
    }
  }
  fclose(fp);
}
