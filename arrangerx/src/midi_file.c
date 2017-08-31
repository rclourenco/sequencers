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
  size_t len;
  MidiEventType type;
}MidiEventNode;

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



int read_track(FILE *fp, size_t len) {
  int i;
  TrackParser tp;
  tp.status = StatusDelta;
  tp.delta  = 0;
  tp.cmd    = 0;
  printf("Track Data:\n");
  for(i=0;i<len;i++) {
    int ch = fgetc(fp);
    if(ch==EOF)
      break;
    tp.ch = ch;
    parse_track(&tp);
/*    printf("%02X ", ch);*/
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
        }
    break;
    case StatusCmd:
      if( (tp->ch & 0x80) != 0 ) {
        tp->orgdatalen = 0;
        switch(tp->ch & 0xF0) {
          case 0x80: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 1; break;
          case 0x90: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 2; break;
          case 0xA0: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 3; break;
          case 0xB0: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 4; break;
          case 0xC0: tp->orgdatalen = 1; tp->status = StatusData; tp->cmd = 5; break;
          case 0xD0: tp->orgdatalen = 1; tp->status = StatusData; tp->cmd = 6; break;
          case 0xE0: tp->orgdatalen = 2; tp->status = StatusData; tp->cmd = 7; break;
          case 0xF0: 
            if( tp->ch == 0xF0 )
               tp->status = StatusSysex;
            else if(tp->ch == 0xF7)
               tp->status = StatusSysex;
            else if(tp->ch == 0xFF)
               tp->status = StatusMeta;
            else
               tp->status = StatusError; 
          break;
          default:
            tp->status = StatusError;
        }
        tp->elen = 0;
        if( tp->status != StatusError ) {
/*          printf("Delta %lu Command: %u %02X\n",tp->delta, tp->cmd, tp->ch);*/
        }
      }
      tp->datalen = tp->orgdatalen;
    break;
    case StatusData:
      if( (tp->ch & 0x80) || tp->datalen == 0) {
        tp->status = StatusError;
      }
      else {
        tp->datalen--;
        if(tp->datalen == 0) {
          tp->status = StatusDelta;
          tp->delta  = 0;
        }
      }
    break;
    case StatusSysex:
      tp->elen = (tp->elen * 128) + (tp->ch & 0x7F);
      if( (tp->ch & 0x80) == 0 ) {
        tp->status = StatusSysexData;
        if( tp->elen > 0 ) {
          tp->status = StatusMetaData;
          printf("@+%08lu SysexData (%lu) [ ", tp->delta, tp->elen);
        }
        else {
          tp->status = StatusDelta;
          printf("@+%08lu SysexData (%lu) [ ]", tp->delta, tp->elen);
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
          printf("@+%08lu %s (%lu) [ ", tp->delta, metalabel(tp->metatype), tp->elen);
        }
        else {
          tp->status = StatusDelta;
          printf("@+%08lu %s", tp->delta, metalabel(tp->metatype));
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
          tp->status = StatusDelta;
          tp->delta  = 0;
        }
      }
    break;
    case StatusMetaData:
      if( tp->elen == 0) {
        printf("] *** Error ***\n");
        tp->status = StatusError;
      }
      else {
        printf("%02X ", tp->ch );
        tp->elen--;
        if(tp->elen == 0) {
          printf("]\n");
          tp->status = StatusDelta;
          tp->delta  = 0;
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
