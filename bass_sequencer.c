#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include "midi_lib/midi_lib.h"
#include <stdlib.h>
#include <stdio.h>

#define SCRW 640
#define SCRH 480

#define MAX_NOTES 64
#define N_STAFFS 3
#define GRID_X 62
#define GRID_Y 20
#define GRID_W 9
#define GRID_H 10
#define NPS    64
#define NNV    25
#define TECLADO_X 110
#define TECLADO_Y 370

#define N_KEYS 20

#define TECLA_W 30
#define TECLA_H 108
#define NO_NOTE 127
#define NOTE_ON 1
#define NOTE_OFF 2


#define STAFFNOTE(X) staffs[cur_staff].notes[(X)]

#define CURNOTE STAFFNOTE(staffs[cur_staff].curnote)
#define NC_SET 0
#define NC_FWD 1

#define STOP 0
#define PLAY 1
#define PAUSE 2
#define PLAY_PAUSE 3

#define DUP1 0
#define DUP4 1
#define DUP8 2


#define CL0 al_map_rgb(0,0,0)
#define CL1 al_map_rgb(0,0,127)
#define CL2 al_map_rgb(0,127,0)
#define CL3 al_map_rgb(0,127,127)
#define CL4 al_map_rgb(127,0,0)
#define CL5 al_map_rgb(127,127,0)
#define CL6 al_map_rgb(127,0,127)
#define CL7 al_map_rgb(127,127,127)

#define CL8 al_map_rgb(80,80,80)
#define CL9 al_map_rgb(0,0,255)
#define CL10 al_map_rgb(0,255,0)
#define CL11 al_map_rgb(0,255,255)
#define CL12 al_map_rgb(255,0,0)
#define CL13 al_map_rgb(255,255,0)
#define CL14 al_map_rgb(255,0,255)
#define CL15 al_map_rgb(255,255,255)


char *GM_names[] = 
   {
      "Acoustic Grand",
      "Bright Acoustic",
      "Electric Grand",
      "Honky-Tonk",
      "Electric Piano 1",
      "Electric Piano 2",
      "Harpsichord",
      "Clav",
      "Celesta",
      "Glockenspiel",
      "Music Box",
      "Vibraphone",
      "Marimba",
      "Xylophone",
      "Tubular Bells",
      "Dulcimer",
      "Drawbar Organ",
      "Percussive Organ",
      "Rock Organ",
      "Church Organ",
      "Reed Organ",
      "Accoridan",
      "Harmonica",
      "Tango Accordian",
      "Acoustic Guitar (nylon)",
      "Acoustic Guitar (steel)",
      "Electric Guitar (jazz)",
      "Electric Guitar (clean)",
      "Electric Guitar (muted)",
      "Overdriven Guitar",
      "Distortion Guitar",
      "Guitar Harmonics",
      "Acoustic Bass",
      "Electric Bass (finger)",
      "Electric Bass (pick)",
      "Fretless Bass",
      "Slap Bass 1",
      "Slap Bass 2",
      "Synth Bass 1",
      "Synth Bass 2",
      "Violin",
      "Viola",
      "Cello",
      "Contrabass",
      "Tremolo Strings",
      "Pizzicato Strings",
      "Orchestral Strings",
      "Timpani",
      "String Ensemble 1",
      "String Ensemble 2",
      "SynthStrings 1",
      "SynthStrings 2",
      "Choir Aahs",
      "Voice Oohs",
      "Synth Voice",
      "Orchestra Hit",
      "Trumpet",
      "Trombone",
      "Tuba",
      "Muted Trumpet",
      "French Horn",
      "Brass Section",
      "SynthBrass 1",
      "SynthBrass 2",
      "Soprano Sax",
      "Alto Sax",
      "Tenor Sax",
      "Baritone Sax",
      "Oboe",
      "English Horn",
      "Bassoon",
      "Clarinet",
      "Piccolo",
      "Flute",
      "Recorder",
      "Pan Flute",
      "Blown Bottle",
      "Skakuhachi",
      "Whistle",
      "Ocarina",
      "Lead 1 (square)",
      "Lead 2 (sawtooth)",
      "Lead 3 (calliope)",
      "Lead 4 (chiff)",
      "Lead 5 (charang)",
      "Lead 6 (voice)",
      "Lead 7 (fifths)",
      "Lead 8 (bass+lead)",
      "Pad 1 (new age)",
      "Pad 2 (warm)",
      "Pad 3 (polysynth)",
      "Pad 4 (choir)",
      "Pad 5 (bowed)",
      "Pad 6 (metallic)",
      "Pad 7 (halo)",
      "Pad 8 (sweep)",
      "FX 1 (rain)",
      "FX 2 (soundtrack)",
      "FX 3 (crystal)",
      "FX 4 (atmosphere)",
      "FX 5 (brightness)",
      "FX 6 (goblins)",
      "FX 7 (echoes)",
      "FX 8 (sci-fi)",
      "Sitar",
      "Banjo",
      "Shamisen",
      "Koto",
      "Kalimba",
      "Bagpipe",
      "Fiddle",
      "Shanai",
      "Tinkle Bell",
      "Agogo",
      "Steel Drums",
      "Woodblock",
      "Taiko Drum",
      "Melodic Tom",
      "Synth Drum",
      "Reverse Cymbal",
      "Guitar Fret Noise",
      "Breath Noise",
      "Seashore",
      "Bird Tweet",
      "Telephone ring",
      "Helicopter",
      "Applause",
      "Gunshot",
      "Acoustic Bass Drum",
      "Bass Drum 1",
      "Side Stick",
      "Acoustic Snare",
      "Hand Clap",
      "Electric Snare",
      "Low Floor Tom",
      "Closed Hi-Hat",
      "High Floor Tom",
      "Pedal Hi-Hat",
      "Low Tom",
      "Open Hi-Hat",
      "Low-Mid Tom",
      "Hi-Mid Tom",
      "Crash Cymbal 1",
      "High Tom",
      "Ride Cymbal 1",
      "Chinese Cymbal",
      "Ride Bell",
      "Tambourine",
      "Splash Cymbal",
      "Cowbell",
      "Crash Cymbal 2",
      "Vibraslap",
      "Ride Cymbal 2",
      "Hi Bongo",
      "Low Bongo",
      "Mute Hi Conga",
      "Open Hi Conga",
      "Low Conga",
      "High Timbale",
      "Low Timbale",
      "High Agogo",
      "Low Agogo",
      "Cabasa",
      "Maracas",
      "Short Whistle",
      "Long Whistle",
      "Short Guiro",
      "Long Guiro",
      "Claves",
      "Hi Wood Block",
      "Low Wood Block",
      "Mute Cuica",
      "Open Cuica",
      "Mute Triangle",
      "Open Triangle"
};

typedef struct {
  ALLEGRO_DISPLAY *display;
  ALLEGRO_FONT *font;
  ALLEGRO_BITMAP *scroll;
  ALLEGRO_EVENT_QUEUE *queue;
  ALLEGRO_EVENT_SOURCE incoming;
  ALLEGRO_MUTEX *mutex;
  ALLEGRO_TIMER *timer;
  int fw;
  int fh;
  int cx;
  int cy;
  char *charbuffer;
} AllegroSet;


typedef struct s_cursor{
    int x;
    int y;
}S_CURSOR;


typedef struct _staff{
    char notes[MAX_NOTES];
    char org_key;
    char key_ofs;
    char div;
    S_CURSOR cursor;
    int curnote;
}STAFF;

char note_set[24];
int note_key[]={ALLEGRO_KEY_A, ALLEGRO_KEY_W, ALLEGRO_KEY_S, ALLEGRO_KEY_E, ALLEGRO_KEY_D, ALLEGRO_KEY_F,
                ALLEGRO_KEY_T, ALLEGRO_KEY_G, ALLEGRO_KEY_Y, ALLEGRO_KEY_H, ALLEGRO_KEY_U, ALLEGRO_KEY_J,
                ALLEGRO_KEY_K, ALLEGRO_KEY_O, ALLEGRO_KEY_L, ALLEGRO_KEY_P, ALLEGRO_KEY_FULLSTOP,
                ALLEGRO_KEY_QUOTE, ALLEGRO_KEY_CLOSEBRACE, ALLEGRO_KEY_BACKSLASH};

STAFF staffs[N_STAFFS];
int cur_staff=0;
int tempo=120;
int oitava=2;
void stuff();
void x_init_staff(STAFF *staff);
void draw_staff(STAFF *staff);

void edit_loop();

void draw_key(int n, char st);

char *note_name[12]={"T","2 m","2 M","3 m",
                      "3 M","IV","IV+","V",
                      "6 m","6 M","7 m","7 M"};
void draw_keyboard();

void process_table_click();
void test_mouse_staff(int *nx, int *ny);

void draw_staffnote(int nx,int ny, int v);
void staff_note_on(int nx, int ny);
int note_ofs=0;

int last_note_on=-1;

void all_notes_off(int channel);

void staffdup(int m);

void playing_cursor(int v);
void player();
int running=0;
void set_player(int st);

void settempo(int v);

volatile int tcount=0;
int save_tcount=0;

void note_on(int channel, int pitch, int vel);
void note_off(int channel, int pitch);
void set_patch(int channel, int prog);
void set_patch2(int channel, int prog);
void set_oitava(int o);

int tr_minor = 0;
int tr_major = 0;

int mouse_x,mouse_y,mouse_b;

void timer_callback_1()
{
/*    tcount++;*/
}
//END_OF_FUNCTION(timer_callback_1)


int init_allegro(AllegroSet *aset)
{
  if(!al_init()) {
     fprintf(stderr, "failed to initialize allegro!\n");
     return 0;
  }

  al_install_keyboard();
  al_install_mouse();
  al_init_font_addon(); // initialize the font addon
  al_init_ttf_addon();// initialize the ttf (True Type Font) addon
  al_init_primitives_addon();

  aset->display = al_create_display(SCRW, SCRH);
  if(!aset->display) {
     fprintf(stderr, "failed to create display!\n");
     return 0;
  }


  aset->timer = al_create_timer(ALLEGRO_BPS_TO_SECS(40.0));
  if(!aset->timer) {
    fprintf(stderr, "Cannot create timer...\n");
    return 0;
  }
/*  aset->scroll = al_create_bitmap(SCRW, SCRH);  */

/*  aset->cx=0;*/
/*  aset->cy=0;*/
/*  aset->fw=16;*/
/*  aset->fh=16;*/
  aset->font = al_load_ttf_font("data/C64_Pro_Mono-STYLE.ttf",8,0 );

  if(!aset->font){
      fprintf(stderr, "Could not load 'C64_Pro_Mono-STYLE.ttf'.\n");
      return 0;
  }

   aset->queue = al_create_event_queue();
   if (!aset->queue) {
      fprintf(stderr,"Error creating event queue\n");
      return 0;
   }
  aset->mutex = al_create_mutex();
  if(!aset->mutex) {
      fprintf(stderr,"Error creating mutex\n");
      return 0;  
  }
  al_init_user_event_source(&aset->incoming);
  al_register_event_source(aset->queue, &aset->incoming);
  al_register_event_source(aset->queue, al_get_keyboard_event_source());
  al_register_event_source(aset->queue, al_get_mouse_event_source());
  al_register_event_source(aset->queue, al_get_timer_event_source(aset->timer));

  return 1;
}


AllegroSet aset;

main(int argc, char **argv)
{
  if(!init_allegro(&aset)) {
    fprintf(stderr, "Error: Cannot init allegro\n");
    exit(2);
  }

  if( argc > 1) {
    midi_port_install(argv[1]);
  } else {
    midi_port_install(NULL);
  }

    stuff();
    return 0;
}
//END_OF_MAIN()



void staffdup(int m)
{
        int i,j,p;
        if(m>NPS||m==0)
                return;
        for(i=1;i<NPS/m;i++)
        {
                for(j=0;j<m;j++)
                {
                        p=i*m+j;
                        if(p<NPS)
                                STAFFNOTE(p)=STAFFNOTE(j);
                }         
        }
        
        draw_staff(&staffs[cur_staff]);
        //all_notes_off(0);
}

void playing_cursor(int v)
{
        int cx=GRID_X+staffs[cur_staff].curnote*GRID_W;
        int cy=GRID_Y+(NNV+1)*GRID_H;
/*        scare_mouse();*/
        al_draw_triangle(cx+GRID_W/2,cy,cx,cy+GRID_H,cx+GRID_W,cy+GRID_H,al_map_rgb(0,0,0),1.0);
        switch(v)
        {
            case NC_SET:staffs[cur_staff].curnote=0;break;
            case NC_FWD:staffs[cur_staff].curnote++;break;
        }
        staffs[cur_staff].curnote=staffs[cur_staff].curnote%NPS;

        cx=GRID_X+staffs[cur_staff].curnote*GRID_W;
        cy=GRID_Y+(NNV+1)*GRID_H;
        al_draw_triangle(cx+GRID_W/2,cy,cx,cy+GRID_H,cx+GRID_W,cy+GRID_H,al_map_rgb(255,0,0),1.0);
/*        unscare_mouse();*/
}


void player()
{

    int base=12*oitava;
    
    
    if((abs(tcount-save_tcount)>=2400/(tempo*4))&&running)
    {
        save_tcount=tcount;
        
        if(last_note_on!=-1)
        {
                note_off(0,last_note_on);
                last_note_on=-1;
        }
        playing_cursor(NC_FWD);
        int temp = CURNOTE;
        if( temp%12 == 4 && tr_minor ) {
          temp--;
        } else if( temp%12 == 3 && tr_major ) {
          temp++;
        }
        last_note_on=temp+base+note_ofs;
        note_on(0,last_note_on,255);
        
    }

    
}

void draw_staffnote(int nx,int ny, int v)
{
    int x=GRID_X+GRID_W*nx;
    int y=GRID_Y+GRID_H*(NNV-1-ny);
/*    scare_mouse();*/
    al_draw_filled_rectangle(x+1,y+1,x+GRID_W-1,y+GRID_H-1, v==NOTE_ON ? CL15 : CL0);
/*    unscare_mouse();*/
}

void staff_note_on(int nx, int ny)
{
     if(nx<0||nx>=NPS||ny<0||ny>=NNV)
        return;
     if(STAFFNOTE(nx)==NO_NOTE)
     {
        STAFFNOTE(nx)=ny; 
        draw_staffnote(nx,ny,NOTE_ON);
     }
     else
     if(STAFFNOTE(nx)!=ny)
     {
         draw_staffnote(nx,STAFFNOTE(nx),NOTE_OFF);
         STAFFNOTE(nx)=ny;
         draw_staffnote(nx,ny,NOTE_ON);
     }   
}

void staff_note_off(int nx, int ny)
{
    if(nx<0||nx>=NPS||ny<0||ny>=NNV)
        return;
    if(STAFFNOTE(nx)==ny)
    {
        draw_staffnote(nx,STAFFNOTE(nx),NOTE_OFF);
        STAFFNOTE(nx)=NO_NOTE;    
    }
}
void process_table_click()
{
        char msg[200]="...";
        int nx=-1,ny=-1;

        test_mouse_staff(&nx,&ny);
        
        sprintf(msg,"Note X: %3d  Note Y: %3d",nx,ny);
        if(mouse_b&0x1)
        {
            staff_note_on(nx,ny);
            //textout(screen,font,msg,20,460,40);
        }
        if(mouse_b&0x2)
        {
               staff_note_off(nx,ny);
        }
}

void test_mouse_staff(int *nx, int *ny)
{
    int max_y,max_x;
    int mx=mouse_x,my=mouse_y;
    
    max_x=GRID_X+NPS*GRID_W;
    max_y=GRID_Y+NNV*GRID_H;
    if(mx>=GRID_X&&mx<=max_x&&my>=GRID_Y&&my<=max_y)
    {
                *nx=(mx-GRID_X)/GRID_W;
                *ny=NNV-(my-GRID_Y)/GRID_H-1;
    }
    else
    {
                *nx=-1;
                *ny=-1;
    }
    
}

void stuff()
{
        al_draw_text(aset.font,CL9,320,5,ALLEGRO_ALIGN_CENTRE,"Monophonic Sequencer");
        x_init_staff(&staffs[0]);
        draw_staff(&staffs[0]);
        draw_keyboard();
        
/*        show_mouse();*/
        playing_cursor(NC_SET);
        edit_loop();
}

void x_init_staff(STAFF *staff)
{
       int i;
       for(i=0;i<MAX_NOTES;i++)
       {
               staff->notes[i]=NO_NOTE;
       }
       staff->cursor.x=0;
       staff->cursor.y=0;
       staff->curnote=0;
       
}

void draw_staff(STAFF *staff)
{
        int i;
        ALLEGRO_COLOR color;
/*        scare_mouse();*/
        al_draw_filled_rectangle(GRID_X,GRID_Y,GRID_X+GRID_W*NPS,GRID_Y+GRID_H*NNV,al_map_rgb(0,0,0));
        for(i=0;i<=NNV;i++)
        {
                al_draw_line(GRID_X,GRID_Y+GRID_H*i,GRID_X+GRID_W*NPS,GRID_Y+GRID_H*i,al_map_rgb(127,0,0),1.0);
        }

        for(i=0;i<=64;i++)
        {
                if(i%4)
                        color=CL4;
                else
                        color=CL7;
                if(i%16==0)
                        color=CL2;
                al_draw_line(GRID_X+GRID_W*i,GRID_Y,GRID_X+GRID_W*i,GRID_Y+GRID_H*NNV,color,1.0);
        }

        for(i=0;i<NNV;i++)
        {
            al_draw_text(aset.font,CL15,GRID_X-45,GRID_Y+GRID_H*(NNV-1-i)+1,0,note_name[i%12]);
        }
        for(i=0;i<NPS;i++)
        {
                if(STAFFNOTE(i)!=NO_NOTE)
                        draw_staffnote(i,STAFFNOTE(i),NOTE_ON);
        }
/*        unscare_mouse();*/
}

void chord_detect()
{
  int i;
  int n=0;
  int treta[4];
  tr_minor = 0;
  tr_major = 0;
  for(i=0;i<N_KEYS;i++) {
    if( n < 4 && note_set[i] ) {
      treta[n]=i;
      n++;
    }
  }

  int bass_note = treta[0];
  for(i=0;i<n;i++) {
    treta[i] -= bass_note;
    printf("T [%u] %d\n", i, treta[i]);
  }

  if( n == 1 ) {
    printf("Major\n");
  }
  else if( n >= 2 ) {
    printf("Diff %d\n", treta[1] - treta[0]);
    if( treta[1] - treta[0] == 4 ) {
      tr_major = 1;
      printf("Major\n");
    }
    else if( treta[1] - treta[0] == 3 ) {
      tr_minor = 1;
      printf("Minor\n");
    }
  }
}

void edit_loop()
{
    ALLEGRO_EVENT event;
    int power_off=0;
    int i;
    int tecla;
    int patch=87;
    int base;

    settempo(120);
    set_patch2(0,patch);
    set_patch(2,87);

    set_oitava(2);

    set_player(STOP);

    al_flip_display();
    al_start_timer(aset.timer);
    while(!power_off)
    {
      al_wait_for_event(aset.queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
          char ch = event.keyboard.unichar;
          switch(ch)
          {
              case '1':settempo(20);break;
              case '2':settempo(40);break;
              case '3':settempo(60);break;
              case '4':settempo(80);break;
              case '5':settempo(100);break;
              case '6':settempo(120);break;
              case '7':settempo(150);break;
              case '8':settempo(180);break;
              case '9':settempo(220);break;
              case '0':settempo(250);break;
          }

/*        power_off = 1;*/
      }
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
          switch(event.keyboard.keycode)
          {
              case ALLEGRO_KEY_ESCAPE: power_off = 1; break;
              case ALLEGRO_KEY_F1: staffdup(1); break;
              case ALLEGRO_KEY_F2: staffdup(4); break;
              case ALLEGRO_KEY_F3: staffdup(8); break;
              case ALLEGRO_KEY_F4: staffdup(16); break;
              case ALLEGRO_KEY_F5: staffdup(32); break;
              case ALLEGRO_KEY_UP: if(patch<127) set_patch2(0,++patch); break;
              case ALLEGRO_KEY_DOWN: if(patch>0) set_patch2(0,--patch); break;
              case ALLEGRO_KEY_PGUP: settempo(tempo+1); break;
              case ALLEGRO_KEY_PGDN: settempo(tempo-1); break;
              case ALLEGRO_KEY_LEFT: set_oitava(oitava-1); break;
              case ALLEGRO_KEY_RIGHT: set_oitava(oitava+1); break;
              case ALLEGRO_KEY_BACKSPACE: set_player(STOP); break;
              case ALLEGRO_KEY_ENTER: set_player(PLAY); break;
              case ALLEGRO_KEY_SPACE: set_player(PLAY_PAUSE); break;
          }

         base=oitava*12;
         for(i=0;i<N_KEYS;i++)
         {
                  if(event.keyboard.keycode == note_key[i] && !note_set[i])
                  {
                    note_set[i]=1;
                    //      note_on(0,base+i,255);
                          note_ofs=i;
                    chord_detect();
                    draw_key(i,1);
                  }

         }

      }
      if( event.type == ALLEGRO_EVENT_KEY_UP ) {
         for(i=0;i<N_KEYS;i++)
         {
            if(event.keyboard.keycode == note_key[i] && note_set[i]) {
                 note_set[i]=0;
                 // note_off(0,base+i);
                 draw_key(i,0);
            }
         }
      }

      if( event.type == ALLEGRO_EVENT_MOUSE_AXES ) {
         mouse_x = event.mouse.x;
         mouse_y = event.mouse.y;
      }

      if( event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN ) {
          int b = event.mouse.button-1;
          mouse_b |= 1<<b;
          process_table_click();
      }

      if( event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP ) {
          int b = event.mouse.button-1;
          mouse_b &= ~(1<<b);
      }

      if( event.type == ALLEGRO_EVENT_TIMER ) {
        tcount++;
        player();
      }
      al_flip_display();
    }
}

void set_patch2(int channel, int prog)
{
/*    scare_mouse();*/
    al_draw_filled_rectangle(20,300,620,310,CL0);
    al_draw_textf(aset.font,CL15,20,300,0,"Instrumento: %u - %-30s",prog,GM_names[prog]);
/*    unscare_mouse();*/
    set_patch(channel,prog);
}


void settempo(int v)
{
  if(v<20||v>1200)
      return;
  tempo=v;

  al_draw_filled_rectangle(20,340,133,350,CL0);
  al_draw_textf(aset.font,CL15,20,340,0,"Tempo: %3u BPM",tempo);
}

void set_patch(int channel, int prog)
{
   unsigned char msg[2];

   msg[0] = 0xC0+channel;
   msg[1] = prog;

//   midi_out(msg, 2);
}

void set_oitava(int o)
{
    if(o<0||o>8)
        return;
    oitava=o;
/*    scare_mouse();*/
    al_draw_filled_rectangle(20,320,130,330,CL0);
    al_draw_textf(aset.font,CL15,20,320,0,"Oitava: %2u",o);
/*    unscare_mouse();*/
}

void set_player(int st)
{
        switch(st)
        {
                case STOP:
                        all_notes_off(0);
                        running=0;
                        playing_cursor(NC_SET);
                        al_draw_filled_rectangle(300,340,380,350,CL0);
                        al_draw_textf(aset.font,CL10,300,340,0,"   STOP  ");
                break;
                case PLAY:
                        running=1;
                        playing_cursor(NC_SET);
                        al_draw_filled_rectangle(300,340,380,350,CL0);
                        al_draw_textf(aset.font,CL9,300,340,0," PLAYING ");
                break;
                case PLAY_PAUSE:
                        if(running)
                        {
                            all_notes_off(0);
                            running=0;
                            al_draw_filled_rectangle(300,340,380,350,CL0);
                            al_draw_textf(aset.font,CL8,300,340,0,"  PAUSE  ");
                        }
                        else
                        {
                            running=1;
                            al_draw_filled_rectangle(300,340,380,350,CL0);
                            al_draw_textf(aset.font,CL9,300,340,0," PLAYING ");
                        }
                break;
        }
}

void set_pan(int channel, int pan)
{
   unsigned char msg[3];

   msg[0] = 0xB0+channel;
   msg[1] = 10;
   msg[2] = pan / 2;

   midi_out(msg, 3);
}



void note_on(int channel, int pitch, int vel)
{
   unsigned char msg[3];

   msg[0] = 0x90+channel;
   msg[1] = pitch;
   msg[2] = vel / 2;

   midi_out(msg, 3);
}



void note_off(int channel, int pitch)
{
   unsigned char msg[3];

   msg[0] = 0x80+channel;
   msg[1] = pitch;
   msg[2] = 0;

   midi_out(msg, 3);
}

void all_notes_off(int channel)
{
    unsigned char msg[3];

    msg[0]=0xB0+channel;
    msg[1]=123;
    msg[2]=0;

    midi_out(msg,3);
}

void draw_key(int n, char st)
{
        int xo;
        int x=TECLADO_X,y=TECLADO_Y;
        int w23=TECLA_W/3*2;
        int w13=TECLA_W/3;
        ALLEGRO_COLOR fc;
        if(st)
                fc=CL10;
        else
                fc=CL15;
        switch(n%12)
        {
               case 0:
                        xo=x;
               break;
               case 2:
                        xo=x+TECLA_W;
               break;
               case 4:
                        xo=x+TECLA_W*2;
               break;
               case 5:
                        xo=x+TECLA_W*3;
               break;
               case 7:
                        xo=x+TECLA_W*4;
               break;
               case 9:
                        xo=x+TECLA_W*5;
               break;
               case 11:
                        xo=x+TECLA_W*6;
               break;
               case 1: xo=x+w23; break;
               case 3: xo=x+TECLA_W+w23; break;
               case 6: xo=x+TECLA_W*3+w23; break;
               case 8: xo=x+TECLA_W*4+w23; break;
               case 10: xo=x+TECLA_W*5+w23; break;
        }

/*        scare_mouse();*/
        
        xo=xo+TECLA_W*7*(n/12);
        switch(n%12)
        {
                case 0:
                case 5:
                        al_draw_filled_rectangle(xo,y+TECLA_H/2,xo+TECLA_W,y+TECLA_H,fc);
                        al_draw_filled_rectangle(xo,y,xo+w23,y+TECLA_H/2,fc);
                        
                        al_draw_line(xo,y,xo+w23,y,CL9,1.0);
                        al_draw_line(xo+w23,y,xo+w23,y+TECLA_H/2,CL9,1.0);
                        al_draw_line(xo+w23,y+TECLA_H/2,xo+TECLA_W,y+TECLA_H/2,CL9,1.0);
                        al_draw_line(xo+TECLA_W,y+TECLA_H/2,xo+TECLA_W,y+TECLA_H,CL9,1.0);
                        al_draw_line(xo,y,xo,y+TECLA_H,CL9,1.0);
                        al_draw_line(xo,y+TECLA_H,xo+TECLA_W,y+TECLA_H,CL9,1.0);
                        break;
                case 2:
                case 7:
                case 9:
                        al_draw_filled_rectangle(xo,y+TECLA_H/2,xo+TECLA_W,y+TECLA_H,fc);
                        al_draw_filled_rectangle(xo+w13,y,xo+w23,y+TECLA_H/2,fc);
                        
                        al_draw_line(xo+w13,y,xo+w23,y,CL9,1.0);
                        al_draw_line(xo+w13,y,xo+w13,y+TECLA_H/2,CL9,1.0);
                        al_draw_line(xo+w23,y,xo+w23,y+TECLA_H/2,CL9,1.0);
                        
                        al_draw_line(xo,y+TECLA_H/2,xo+w13,y+TECLA_H/2,CL9,1.0);
                        al_draw_line(xo+w23,y+TECLA_H/2,xo+TECLA_W,y+TECLA_H/2,CL9,1.0);
                        al_draw_line(xo,y+TECLA_H/2,xo,y+TECLA_H,CL9,1.0);
                        al_draw_line(xo+TECLA_W,y+TECLA_H/2,xo+TECLA_W,y+TECLA_H,CL9,1.0);
                        al_draw_line(xo,y+TECLA_H,xo+TECLA_W,y+TECLA_H,CL9,1.0);
                        break;
                case 4:
                case 11:

                        al_draw_filled_rectangle(xo,y+TECLA_H/2,xo+TECLA_W,y+TECLA_H,fc);
                        al_draw_filled_rectangle(xo+w13,y,xo+TECLA_W,y+TECLA_H/2,fc);
                        
                        al_draw_line(xo,y+TECLA_H/2,xo+w13,y+TECLA_H/2,CL9,1.0);
                        al_draw_line(xo+w13,y+TECLA_H/2,xo+w13,y,CL9,1.0);
                        al_draw_line(xo+w13,y,xo+TECLA_W,y,CL9,1.0);
                        al_draw_line(xo,y+TECLA_H/2,xo,y+TECLA_H,CL9,1.0);
                        al_draw_line(xo+TECLA_W,y,xo+TECLA_W,y+TECLA_H,CL9,1.0);
                        al_draw_line(xo,y+TECLA_H,xo+TECLA_W,y+TECLA_H,CL9,1.0);        
                        break;
                case 1:
                case 3:
                case 6:
                case 8:
                case 10:
                        if(!st)
                                fc=CL0;
                        al_draw_filled_rectangle(xo,y,xo+w23,y+TECLA_H/2,fc);
                        
                        al_draw_rectangle(xo,y,xo+w23,y+TECLA_H/2,CL9,1.0);
                        break;
               
        }

/*        unscare_mouse();*/
}


void draw_keyboard()
{
       int i;
       for(i=0;i<24;i++)
       {
           draw_key(i,0);
       }
}

