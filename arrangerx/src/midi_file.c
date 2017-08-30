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
    else {
      if(offset < 0)
        break;
      if(fseek(fp,offset,SEEK_CUR))
        break;
    }
  }
  fclose(fp);
}
