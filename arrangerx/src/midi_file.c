#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "midi_pattern.h"
#include "../../midi_lib/midi_lib.h"

int store_event_data( TrackParser *tp, unsigned char data )
{

  if(!tp->current)
    return 0; /*error*/

  if( tp->datalen ) {
    if( tp->current->datalen < tp->orgdatalen ) {
      tp->current->data[tp->current->datalen++] = data;
    } else {
      fprintf(stderr, "Ignoring data... Reserved Len: %lu, Current %lu\n", tp->orgdatalen, tp->current->datalen);
    }
    tp->datalen--;
    return 1;
  }

  fprintf(stderr, "Data overflow... .!.\n");

  return 0; /*Error*/
}

int store_meta_type(TrackParser *tp)
{
  if(tp->current) {
    if( tp->current->type == meMeta )
      tp->current->t.meta.type = tp->metatype;
    return 1;
  }
  return 0;
}

int store_sysex_type(TrackParser *tp)
{
  if(tp->current) {
    if( tp->current->type == meSysex )
      tp->current->t.sysex.type = tp->metatype;
    return 1;
  }
  return 0;
}


int store_sysex_length(TrackParser *tp)
{
  if(tp->current) {
    if( tp->current->type == meSysex )
      tp->current->t.sysex.length = tp->elen;
    return 1;
  }
  return 0;
}

int store_meta_length(TrackParser *tp)
{
  if(tp->current) {
    if( tp->current->type == meMeta )
      tp->current->t.meta.length = tp->elen;
    return 1;
  }
  return 0;
}


int new_event_node(TrackParser *tp, MidiEventType type )
{
  MidiEventNode *new = (MidiEventNode *)malloc(sizeof(MidiEventNode) + tp->orgdatalen*sizeof(unsigned char));
  if(!new)
    return 0;

  tp->datalen = tp->orgdatalen;

  if(!tp->first) {
    tp->first = new;
  }
  else {
    tp->current->next = new;
  }
  tp->current = new;
  new->next = NULL;

  new->type = type;
  new->time = tp->time;
  new->datalen = 0;
  
  
  switch(type)
  {
    case meCV:
    case meCVRunning:
        new->t.vc.channel = tp->channel;
        new->t.vc.type    = tp->cmd;
    break;
    case meCommon:
      new->t.common.type = tp->ch;
    break;
    case meSystem:
      new->t.system.type = tp->ch;
    break;
    case meSysex:
      new->t.sysex.length = tp->elen;
      new->t.sysex.type   = tp->metatype;
    break;
    default:
      new->t.meta.length = tp->elen;
      new->t.meta.type   = tp->metatype;
  }
  return 1;
}

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
    case 8: st = "Program Name"; break;
    case 9: st = "Device Name"; break;
    case 0x20: st = "Midi Channel Prefix"; break;
    case 0x21: st = "MIDI Port"; break;
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

void dump_meta(MidiEventNode *en)
{
  printf("%-20s", metalabel(en->t.meta.type));
  if( ! en->datalen )
    return;

  if( en->t.meta.type == 0 || en->t.meta.type > 9) {
    printf(" [");
    int i;
    for(i=0;i<en->datalen;i++) {
      if(i) putchar(' ');
      printf("%02X", en->data[i]);
    }
    printf("]");
  }
  else {
    printf(" '");
    int i;
    for(i=0;i<en->datalen;i++) {
      if( en->data[i] >= 32 && en->data[i] < 127 ) {
        putchar(en->data[i]);
      }
      else {
        printf("<%02X>", en->data[i]);
      }
    }
    putchar('\'');
  }
}

void dump_sysex(MidiEventNode *en)
{
  printf("Sysex_%02x ", en->t.sysex.type);
  int i;
  printf("[");
  for(i=0;i<en->datalen;i++) {
    if(i) putchar(' ');
    printf("%02X", en->data[i]);
  }
  printf("]");
}

void dump_common(MidiEventNode *en)
{
/*  printf("[");*/
/*  int i;*/
/*  for(i=0;i<en->datalen;i++) {*/
/*    if(i) putchar(' ');*/
/*    printf("%02X");*/
/*  }*/
/*  printf("]");*/
}

void dump_system(MidiEventNode *en)
{
}


void dump_channel_voice(MidiEventNode *en) {
  char rs = en->type == meCVRunning ? 'r':'n';
  switch( en->t.vc.type ) {
    case 1: printf("NoteOff           %c ch:%2u note:%3u velocity:%3u", rs, en->t.vc.channel, en->data[0],en->data[1]); break;
    case 2: printf("NoteOn            %c ch:%2u note:%3u velocity:%3u", rs, en->t.vc.channel, en->data[0],en->data[1]); break;
    case 3: printf("PolyAftertouch    %c ch:%2u note:%3u pressure:%3u", rs, en->t.vc.channel, en->data[0],en->data[1]); break;
    case 4: printf("ControllerChange  %c ch:%2u control:%3u value:%3u", rs, en->t.vc.channel, en->data[0],en->data[1]); break;
    case 5: printf("ProgramChange     %c ch:%2u program:%3u",  rs, en->t.vc.channel, en->data[0]); break;
    case 6: printf("ChannelAftertouch %c ch:%2u pressure:%3u", rs, en->t.vc.channel, en->data[0]); break;
    case 7: printf("PitchBend         %c ch:%2u level:%04Xh  ",  rs, en->t.vc.channel, (en->data[0]+en->data[1]*128)); break;
  }
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
    printf("Division: %02X %02X\n", mfh.division[0], mfh.division[1]);
    if( mfh.division[0] & 0x80 ) {
      header->ticks_frame = mfh.division[1];
      char x = (mfh.division[0] | 0x80);
      printf("frames_second neg %d\n", x);
      header->frames_second = -x;
      header->ticks_quarter = 0;
    }
    else {
      header->ticks_quarter = (mfh.division[0]&0x7F)*256+mfh.division[1];
      header->ticks_frame   = 0;
      header->frames_second = 0;
    }
    return 1;
  }
  return 0;
}


void init_track_parser(TrackParser *tp) {
  tp->first   = NULL;
  tp->current = NULL;

  tp->status = StatusDelta;
  tp->delta  = 0L;
  tp->cmd    = 0;
  tp->time   = 0L;
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
  if(tp->time > 100000 ) {
    printf("OOOOOOOOOOOOOOOOOOO\n");
  }
  tp->time += tp->delta;
  return 0;
}

void parse_new_delta(TrackParser *tp)
{
  tp->status = StatusDelta;
  tp->delta  = 0L;
}

void dump_track_events( MidiEventNode *first )
{
  MidiEventNode *cur = first;
  while(cur) {
    switch( cur->type )
    {
      case meCV:
      case meCVRunning: printf("@%010lu ", cur->time); dump_channel_voice(cur);putchar('\n'); break;
      case meCommon:    printf("@%010lu Common ", cur->time); dump_common(cur); putchar('\n'); break;
      case meSystem:    printf("@%010lu System ", cur->time); dump_system(cur); putchar('\n'); break;
      case meSysex:     printf("@%010lu ", cur->time); dump_sysex(cur); putchar('\n'); break;
      case meMeta:      printf("@%010lu ", cur->time); dump_meta(cur); putchar('\n'); break;
    }
    cur = cur->next;
  }
}

MidiEventNode *read_track(FILE *fp, size_t len)
{
  int i;
  TrackParser tp;
  init_track_parser(&tp);

  for(i=0;i<len;i++) {
    int ch = fgetc(fp);
    if(ch==EOF)
      break;
    tp.ch = ch;
    parse_track(&tp);
  }

  if( tp.status != StatusError && tp.first ) {
    return tp.first;
  }
  return NULL;
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
        MidiEventType type = meCV;
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
              case 0xF0: tp->status = StatusSysex; tp->cmd = 0; type = meSysex; tp->metatype = tp->ch; break;
              case 0xF1: tp->cmd = 0; parse_new_delta(tp); type = meCommon; break;
              case 0xF2: /*Song Position Pointer*/ tp->status = StatusData; tp->orgdatalen = 2; tp->cmd = 0; type = meCommon; break;
              case 0xF3: /*Song Select*/ tp->status = StatusData; tp->orgdatalen = 1; tp->cmd = 0; type = meCommon; break;
              case 0xF4: /**/ tp->cmd = 0; parse_new_delta(tp); type = meCommon; break;
              case 0xF5: /**/ tp->cmd = 0; parse_new_delta(tp); type = meCommon; break;
              case 0xF6: /*Tune Request*/ tp->cmd = 0; parse_new_delta(tp); type = meCommon; break;
              case 0xF7: tp->status = StatusSysex; tp->cmd = 0; type = meSysex; tp->metatype = tp->ch; break;
              case 0xF8: /*Timing Clock*/ tp->orgdatalen = 0; parse_new_delta(tp); type = meSystem; break;
              case 0xF9: /*Unknown*/      tp->orgdatalen = 0; parse_new_delta(tp); type = meSystem; break;
              case 0xFA: /*Start*/        tp->orgdatalen = 0; parse_new_delta(tp); type = meSystem; break;
              case 0xFB: /*Stop*/         tp->orgdatalen = 0; parse_new_delta(tp); type = meSystem; break;
              case 0xFC: /*Unknown*/      tp->orgdatalen = 0; parse_new_delta(tp); type = meSystem; break;
              case 0xFD: /*Unknown*/      tp->orgdatalen = 0; parse_new_delta(tp); type = meSystem; break;
              case 0xFE: /*Active Sensing*/ tp->orgdatalen = 0; parse_new_delta(tp); type = meSystem; break;
              case 0xFF: tp->status = StatusMeta; tp->cmd = 0; type = meMeta; break;
              default: tp->cmd = 0; tp->orgdatalen = 0; parse_new_delta(tp);
            }
          break;
          default:
            tp->status = StatusError;
        }
        tp->elen = 0;
        if( type == meCV && tp->status != StatusError ) {
          if( ! new_event_node(tp, type) ) {
            fprintf(stderr, "Cannot alloc Event memory\n");
            tp->status = StatusError;
            return;
          }
        }
      }
      else {
        if( tp->orgdatalen == 0 || !tp->cmd ) {
            /* Ignore these data bytes*/
            /*tp->status = StatusError;*/
        }
        else {
          if(! new_event_node(tp, meCVRunning) ) {
            tp->status = StatusError;
            return;
          }
          store_event_data(tp, tp->ch);
          if(tp->datalen == 0) {
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
        store_event_data(tp, tp->ch);
        if(tp->datalen == 0) {
          parse_new_delta(tp);
        }
      }
    break;
    case StatusSysex:
      tp->elen = (tp->elen * 128) + (tp->ch & 0x7F);
      if( (tp->ch & 0x80) == 0 ) {
        if( tp->elen > 0 ) {
          tp->orgdatalen = tp->elen;
          if(! new_event_node(tp, meSysex) ) {
            fprintf(stderr, "Cannot alloc Event memory\n");
            tp->status = StatusError;
            return;
          }
          tp->status = StatusSysexData;
        }
        else {
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

        tp->orgdatalen = tp->elen;
        if(! new_event_node(tp, meMeta) ) {
          fprintf(stderr, "Cannot alloc Event memory\n");
          tp->status = StatusError;
          return;
        }

        if( tp->datalen) {
          tp->status = StatusMetaData;
        }
        else {
          parse_new_delta(tp);
        }
      }
    break;
    case StatusSysexData:
      if( tp->elen == 0) {
        tp->status = StatusError;
      }
      else {
        store_event_data(tp, tp->ch);
        tp->elen--;
        if(tp->elen == 0) {
          parse_new_delta(tp);
        }
      }
    break;
    case StatusMetaData:
      if( tp->elen == 0) {
        tp->status = StatusError;
      }
      else {
        store_event_data(tp, tp->ch);
        tp->elen--;
        if(tp->elen == 0) {
          parse_new_delta(tp);
        }
      }
    break;
  }
}


void dump_pattern(MidiPattern *pat)
{
  MidiTrackNode *current = pat->tracks;
  int i = 0;
  while(current) {
    printf("Track: %u\n", i);
    dump_track_events(current->events);
    printf("*********************************\n");

    current = current->next;
    i++;
  }
}

MidiPattern *midi_pattern_load(char *filename)
{
  FILE *fp;
  fp = fopen(filename,"rb");
  if(!fp) {
    fprintf(stderr,"Cannot open file %s\n", filename);
    return NULL;
  }

  MidiPattern *pat = (MidiPattern *)malloc(sizeof(MidiPattern));

  if( pat ) {
    MidiPrefix mp;
    MidiHeader mh;

    pat->tracks = NULL;
    strcpy(pat->filename, filename);
    pat->ticks_quarter = 0;

    MidiTrackNode *current = NULL;
    int expected_tracks = 0;
    int parsed_tracks   = 0;
    while( read_prefix(fp,&mp) ) {
      printf("Type %s Len: %u\n", mp.type, (unsigned int)mp.length);
      long offset = mp.length;
      if( !strcmp(mp.type,"MThd") && mp.length == 6 ) {
        if(!read_header(fp, &mh)) {
          break;
        }
        printf("Format %u Tracks %u ", mh.format, mh.tracks );
        if( mh.ticks_quarter ) {
           pat->ticks_quarter = mh.ticks_quarter;
        } else{
          printf("Frames/second: %d Ticks/Frame: %d\n", mh.frames_second, mh.ticks_frame );
        }
        expected_tracks = mh.tracks;
      }
      else if( !strcmp(mp.type,"MTrk") ) {
        MidiTrackNode *new = (MidiTrackNode *)malloc(sizeof(MidiTrackNode));
        if(new) {
          MidiEventNode *first = read_track(fp, mp.length);
          if( first ) {
            new->events = first;
            new->playing = NULL;
            new->next    = NULL;
            if( current ) {
              current->next = new;
            }
            current = new;
            if( ! pat->tracks ) {
              pat->tracks = new;
            }
            parsed_tracks++;
          } else {
            printf("Error importing track...\n");
            break;
          }
        }
      }
      else {
        if(offset < 0)
          break;
        if(fseek(fp,offset,SEEK_CUR))
          break;
      }
    }

    if( expected_tracks != parsed_tracks ) {
      /*TODO free pattern memory */
      free(pat);
      pat = NULL;
    }
  }

  fclose(fp);
  return pat;
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
      if(en->datalen==1) {
        msg[0]=en->data[0];
        midi_out(msg,1);
      }
      else if(en->datalen==2) {
        msg[0]=en->data[0];
        msg[1]=en->data[1];
        midi_out(msg,2);
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
      execute_midi_event(current->playing);
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
    printf("Cannot play MIDI Files with SMTPE timing");
    return;
  }
  unsigned long quarter_interval = 705880;
  unsigned long ticks_interval = quarter_interval/pat->ticks_quarter;

  struct timespec tp;
  unsigned long usec_el, o_usec_el,adiff;

  clock_gettime(CLOCK_REALTIME, &tp);
  o_usec_el = tp.tv_nsec/1000 + (tp.tv_sec%1000) * 1000000;

  unsigned long i;
  unsigned long osec = 10000;
  for(i=0;i<0xFFFFF;i++) {
    clock_gettime(CLOCK_REALTIME, &tp);
    usec_el = tp.tv_nsec/1000 + (tp.tv_sec%1000) * 1000000;
    if(usec_el < o_usec_el)
      adiff = abs(1000000000+usec_el - o_usec_el);
    else
      adiff = abs(usec_el - o_usec_el);

    unsigned long tick = adiff/ticks_interval;

    unsigned long adiff2 = (tick+1)*ticks_interval;

    int done = play_pattern(pat,tick);
    unsigned long secs = adiff/1000000;
    if( osec != secs ) {
      printf("\rTime: %02u:%02u:%02u", (unsigned int)(secs/3600)%24,(unsigned int) (secs/60)%60, (unsigned int)secs%60);
      fflush(stdout);
      osec = secs;
    }
    if(done)
      break;
    usleep(adiff2-adiff);
  }

  printf("\nDone.\n\n");
}

main(int argc, char **argv)
{
  if( argc < 2) {
    return 1;
  }

  MidiPattern *patm = midi_pattern_load(argv[1]);
  if( patm ) {
    printf("Import Ok\n");
/*    dump_pattern(patm);*/
  }

  midi_port_install("alsa;out:hw:2,0");

  printf("Playing %s\n", patm->filename);
  rewind_pattern(patm);
  playing_loop(patm);
  
}
