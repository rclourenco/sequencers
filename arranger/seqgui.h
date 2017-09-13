#ifndef SEQGUI_H

#define SEQGUI_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#define SCRW 800
#define SCRH 600

typedef struct {
  ALLEGRO_DISPLAY *display;
  ALLEGRO_FONT *font;
  ALLEGRO_BITMAP *scroll;
  ALLEGRO_EVENT_QUEUE *queue;
  ALLEGRO_EVENT_SOURCE incoming;
  ALLEGRO_MUTEX *mutex;
  ALLEGRO_TIMER *timer;
  int sw,sh;
  int fw;
  int fh;
  int cx;
  int cy;
  char *charbuffer;
} AllegroSet;

int init_allegro(AllegroSet *aset);
void run_dialog();
void init_gui(AllegroSet *aset);

#endif
