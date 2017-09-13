#include <stdio.h>
#include "glux.h"
#include "seqgui.h"
#include "seq.h"

char the_string[64] = "0.0";


#include <time.h>
struct tm the_time;


typedef struct{
	int bpm;
	int div;
	int pulse;
	int frag;
	int seq_count;
	int chord_count;
	int  notes_n;
	int  note_p;
}SeqView;


typedef struct{
	char song_name[256];
	int group_count;

	int intro_group;
	int outro_group;
	int enter_group;

	int cur_group;
	SeqView seqs[MAXGROUPS];

	int playstatus;
	int state;
	int defaultnext;
	int next;
}SongView;

void copy_seq_data(SongView *songview, SeqData *sdv)
{
	strcpy(songview->song_name,sdv->song.song_name);
	songview->group_count = sdv->song.group_count;
	songview->cur_group   = sdv->seqstatus_current.group;
	songview->playstatus  = sdv->seqstatus_current.play;
	songview->state       = sdv->seqstatus_current.state;
	songview->defaultnext = sdv->song.state[ sdv->seqstatus_current.state ].next;
	songview->next        = sdv->seqstatus_current.nextstate;
	int i;
	int l = songview->group_count;
	if(l>MAXGROUPS)
		l = MAXGROUPS;

	for(i=0;i<l;i++){
		SeqGroup *sg = &sdv->song.group[i];
		songview->seqs[i].bpm         = sg->bpm;
		songview->seqs[i].div         = sg->div;
		songview->seqs[i].pulse       = sg->pulse;
		songview->seqs[i].frag        = sg->frag;
		songview->seqs[i].seq_count   = sg->seq_count;
		songview->seqs[i].chord_count = sg->chord_count;
		songview->seqs[i].notes_n     = sg->notes_n;
		songview->seqs[i].note_p      = sg->note_p;
	}
}

SongView songstatus, oldsongstatus;
/* custom dialog procedure for the clock object */
int status_update(int msg, DIALOG *d, int c)
{
    char buffer[200];
/*   time_t current_time;*/
/*   struct tm *t;*/
/*   BITMAP *temp;*/
/*   fixed angle, x, y;*/
    float x;
		int ls = 28;
		int lp = 1;


   /* process the message */
   switch (msg) {

      /* initialise when we get a start message */
      case MSG_START:
		pthread_mutex_lock(&count_mutex);
		copy_seq_data(&songstatus,&sdata);
		pthread_mutex_unlock(&count_mutex);
/*		memcpy(&oldsongstatus,&songstatus,sizeof(SongView));*/

	 /* draw the clock background onto a memory bitmap */
/*	 temp = create_bitmap(d->w, d->h);*/
/*	 clear_to_color(temp, d->bg);*/

	 /* store the clock background bitmap in d->dp */
/*	 d->dp = temp;*/
	 break;

      /* shutdown when we get an end message */
      case MSG_END:
	 /* destroy the clock background bitmap */
/*	 destroy_bitmap(d->dp);*/
	 break;

      /* update the clock in response to idle messages */
      case MSG_IDLE:
		pthread_mutex_lock(&count_mutex);
		copy_seq_data(&songstatus,&sdata);
		pthread_mutex_unlock(&count_mutex);
		if( memcmp(&oldsongstatus,&songstatus, sizeof(SongView)) ) {
			memcpy(&oldsongstatus,&songstatus,sizeof(SongView));
			/* Redraw ourselves if the time has changed. Note that the dialog
			 * manager automatically turns off the mouse pointer whenever a
			 * MSG_DRAW message is sent to an individual object or an entire
			 * dialog, so we don't have to do it explicitly. Also note the use
			 * of the object_message function rather than a simple recursive
			 * call to clock_proc(). This vectors the call through the function
			 * pointer in the dialog object, which allows other object
			 * procedures to hook it, for example a different type of clock
			 * could process the draw messages itself but pass idle messages
			 * on to this procedure.
			 */
		    object_message(d, MSG_DRAW, 0);
		 }
	 break;

      /* draw the clock in response to draw messages */
      case MSG_DRAW:
		#define CL (ls*lp)
		#define NL (ls*lp++)
		#define COL1 5
		#define COL2 300
		#define FL()   lp=1

	 /* draw onto a temporary memory bitmap to prevent flicker */
	  al_draw_filled_rectangle(d->x, d->y, d->w, d->h, d->bg);
	  al_draw_rectangle(d->x, d->y, d->w, d->h, d->fg, 1.0);

    sprintf(buffer, "Song name:        %s", songstatus.song_name);
    al_draw_text(algui.font, al_map_rgb(255,0,200), COL1, NL, 0,buffer);

/*		textprintf_ex(temp, font, COL1, NL, makecol(255, 0, 200),*/
/*		                -1, "Song name:        %s", songstatus.song_name);*/

		if(songstatus.playstatus == STOP) {
/*			textprintf_ex(temp, font, COL1, NL, makecol(255, 0, 0),*/
/*		                -1, "STOP");*/
      al_draw_text(algui.font, al_map_rgb(255,0, 0), COL1, NL, 0, "STOP");
		}
		else if(songstatus.playstatus == PLAY ) {
/*			textprintf_ex(temp, font, COL1, NL, makecol(0, 255, 0),*/
/*		                -1, "PLAYING");*/
      al_draw_text(algui.font, al_map_rgb( 0, 255, 0), COL1, NL, 0, "PLAYING");
		}

		NL;
/*			textprintf_ex(temp, font, COL1, CL, makecol(255, 255, 255),*/
/*		                -1, "CS: %d",	songstatus.state);*/

      sprintf(buffer,"CS: %d",	songstatus.state);
      al_draw_text(algui.font, al_map_rgb( 255, 255, 255), COL1, CL, 0, buffer);

/*			textprintf_ex(temp, font, COL2, CL, makecol(255, 255, 255),*/
/*		                -1, "NEXT: %d",	songstatus.next >= 0 ? songstatus.next : 	songstatus.defaultnext);*/
            sprintf(buffer,"NEXT: %d",	songstatus.next >= 0 ? songstatus.next : 	songstatus.defaultnext);
            al_draw_text(algui.font, al_map_rgb( 255, 255, 255), COL2, CL, 0, buffer);


		int frag = songstatus.seqs[songstatus.cur_group].frag;
		int div = songstatus.seqs[songstatus.cur_group].div;
		if(div == 0)
			div = 1;
		int beats = songstatus.seqs[songstatus.cur_group].frag / div;
		if(beats == 0)
			beats = 1;

		int bars  = songstatus.seqs[songstatus.cur_group].notes_n / (beats * div);
		int notes = songstatus.seqs[songstatus.cur_group].notes_n;

		int pos  = songstatus.seqs[songstatus.cur_group].note_p;
		int beat = (pos / div) % beats;
		int bar  = (pos/div) / beats;
		int bt   = (pos/div);
		int bts  = notes/div;
		
		int dx = 160;
		int dim = 770-dx;
		
		int beatp    = ( (beat+1) * dim)/beats;
		int beatp0   = (beat * dim)/beats;

		int barp  = ((bar+1) * dim)/bars;
		int barp0  = (bar * dim)/bars;
		int barm = (barp + barp0)/2;

		int posp  = ((pos+1) * dim)/notes;
		int posp0  = (pos * dim)/notes;
		int posm = (posp + posp0)/2;

		int btp  = ( (bt+1) * dim)/bts;
		int btp0  = (bt * dim)/bts;
		int btm = (btp + btp0)/2;



		NL;

		al_draw_rectangle(dx-1, lp*ls+3, dx+dim+1, (lp+1)*ls-3,al_map_rgb(255, 255, 255), 1.0);
		al_draw_rectangle(dx-1, (lp+1)*ls+3, dx+dim+1, (lp+2)*ls-3,al_map_rgb(255, 255, 255),1.0);

		al_draw_filled_rectangle(dx+beatp0, lp*ls+4, dx+beatp, (lp+1)*ls-4,al_map_rgb(255, 255,0));


    sprintf(buffer, "Beat: %4d/%-4d", beat+1,beats);
    al_draw_text(algui.font, al_map_rgb( 255, 0, 200), COL1, NL, 0, buffer);

/*		textprintf_ex(temp, font, COL1, NL, makecol(255, 0, 200),*/
/*		                -1, "Beat: %4d/%-4d", beat+1,beats);*/

		al_draw_filled_rectangle(dx+barm-2, lp*ls+6, dx+barm+2, lp*ls+10,     al_map_rgb(255, 0, 0));
		al_draw_filled_rectangle(dx+btm-2, lp*ls+12, dx+btm+2, lp*ls+16,      al_map_rgb(0, 255, 0));
		al_draw_filled_rectangle(dx+posm-2, lp*ls+18, dx+posm+2, (lp+1)*ls-6, al_map_rgb(0, 0, 255));

    sprintf(buffer, "Bar:  %4d/%-4d", bar+1,bars);
    al_draw_text(algui.font, al_map_rgb( 255, 0, 200), COL1, NL, 0, buffer);


/*		textprintf_ex(temp, font, COL1, NL, makecol(255, 0, 200),*/
/*		                -1, "Bar:  %4d/%-4d", bar+1,bars);*/

		lp++;
		int startline = lp*ls;
		al_draw_text(algui.font, al_map_rgb( 0, 0, 160), COL1, NL, 0,
		 " Sequence          Voices     BPM    Beats    Bars    Next    Trigger  ");
/*		textprintf_ex(temp, font, COL1, NL, makecol(255, 255, 255),*/
/*		                makecol(0,0,160), */
/*			" Sequence          Voices     BPM    Beats    Bars    Next    Trigger  ",*/
/*		 pos);*/
		int midline = lp*ls;
		int i;
		for( i=0; i<songstatus.group_count; i++ )
		{
			ALLEGRO_COLOR color;
			int bg;

			int line = NL;

			if( i == songstatus.cur_group ) {
				
				color = al_map_rgb(255, 255, 255);
				al_draw_filled_rectangle(5, line, 645, line+ls,al_map_rgb(0, 160, 0));
			}
			else {
				color = al_map_rgb(0, 160, 0);
			}
			
			int frag = songstatus.seqs[i].frag;
			int div = songstatus.seqs[i].div;
			if(div == 0)
				div = 1;
			int beats = songstatus.seqs[i].frag / div;
			if(beats == 0)
				beats = 1;

			int bars  = songstatus.seqs[i].notes_n / (beats * div);

        sprintf(buffer, " Seq.  %5d",i);
        al_draw_text(algui.font,color, 5, line, 0, buffer);
/*			textprintf_ex(temp, font, 5, line, color,*/
/*		                -1," Seq.  %5d",i);*/

        sprintf(buffer, " %9d",songstatus.seqs[i].seq_count);
        al_draw_text(algui.font, color, 150, line, 0, buffer);
/*			textprintf_ex(temp, font, 150, line, color,*/
/*		                -1," %9d",songstatus.seqs[i].seq_count);*/
        sprintf(buffer, " %7d",songstatus.seqs[i].bpm);
        al_draw_text(algui.font, color, 246, line, 0, buffer);
/*			textprintf_ex(temp, font, 246, line, color,*/
/*		                -1," %7d",songstatus.seqs[i].bpm);*/
        sprintf(buffer, " %5d",beats);
        al_draw_text(algui.font, color, 326, line, 0, buffer);
/*			textprintf_ex(temp, font, 326, line, color,*/
/*		               -1," %5d",beats);*/
        sprintf(buffer, " %5d",bars);
        al_draw_text(algui.font, color, 405, line, 0, buffer);
/*			textprintf_ex(temp, font, 405, line, color,*/
/*		                -1," %5d",bars);*/
        sprintf(buffer, " %s","<=0=>");
        al_draw_text(algui.font, color, 475, line, 0, buffer);
/*			textprintf_ex(temp, font, 475, line, color,*/
/*		                -1," %s","<=0=>");*/
        sprintf(buffer, " < %d >",i);
        al_draw_text(algui.font, color, 548, line, 0, buffer);
/*			textprintf_ex(temp, font, 548, line, color,*/
/*		                -1," < %d >",i);*/

//			" Sequence          Voices     BPM    Beats    Bars    Next    Trigger
		}

		int endline = lp*ls;
		ALLEGRO_COLOR white = al_map_rgb(255,255,255);
		al_draw_line(5,startline,645,startline,    white,1.0);
		al_draw_line(5,midline,645,midline,        white,1.0);
		al_draw_line(5,   startline, 5,   endline, white,1.0);
		al_draw_line(150, startline, 150, endline, white,1.0);
		al_draw_line(246, startline, 246, endline, white,1.0);
		al_draw_line(326, startline, 326, endline, white,1.0);
		al_draw_line(405, startline, 405, endline, white,1.0);
		al_draw_line(475, startline, 475, endline, white,1.0);
		al_draw_line(548, startline, 548, endline, white,1.0);
		al_draw_line(645, startline, 645, endline, white,1.0);
		al_draw_line(5,   endline,   645, endline, white,1.0);

/*		 blit(temp, screen, 0, 0, d->x, d->y, d->w, d->h);*/
/*		*/
/*	*/

/*	 destroy_bitmap(temp);*/
	 break; 
   }

   /* always return OK status, since the clock doesn't ever need to close 
    * the dialog or get the input focus.
    */
   return D_O_K;
}




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
  aset->sw = SCRW;
  aset->sh = SCRH;

  aset->display = al_create_display(aset->sw, aset->sh);
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
/*  al_init_user_event_source(&aset->incoming);*/
/*  al_register_event_source(aset->queue, &aset->incoming);*/
  al_register_event_source(aset->queue, al_get_keyboard_event_source());
  al_register_event_source(aset->queue, al_get_mouse_event_source());
  al_register_event_source(aset->queue, al_get_timer_event_source(aset->timer));

  return 1;
}



void init_gui(AllegroSet *aset)
{
  GuiAllegroSetup algui;
  algui.display = aset->display;
  algui.font    = aset->font;
  algui.queue   = aset->queue;
  algui.w       = aset->sw;
  algui.h       = aset->sh;
  al_gui_init(&algui);
/*  algui.incoming;*/
/*  algui.mutex;*/
/*  algui.timer;*/
}

#define WHITE {.r = 255, .b= 255, .g = 255}

DIALOG the_dialog[] =
{
/*    (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */
   { d_clear_proc,        0,   0,    0,    0,    {},   {},    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   { d_ctext_proc,        0,   0,    800,    40, {},   {},    0,      0,       0,   0,    (void*)"The Savage Sequencer",  NULL, NULL  },
   { d_button_proc,     328,   560, 160,   24,   {},   {}, 0, D_EXIT,       0,   0,    (void*)"Close",         NULL, NULL  },
   { status_update,       10,  40,  790,  400,  WHITE, {},    0,    0,       0,   0,    NULL,          NULL, NULL  },
   { d_yield_proc,        0,   0,    0,    0,   {},    {},    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   { NULL,                0,   0,    0,    0,   {},    {},    0,      0,       0,   0,    NULL,                   NULL, NULL  }
};

/*DIALOG the_dialog[] =*/
/*{*/
   /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */
/*    { d_clear_proc,        0,   0,   0,   0,   {},  {},    0,      0,       0,   0,    NULL,                           NULL, NULL  },*/
/*    { d_ctext_proc,        0,   0, 800,  40,    0,   0,    0,      0,       0,   0,    (void*)"The Savage Sequencer",  NULL, NULL  },*/
/*    { d_button_proc,     328, 260, 160,  24,   {},  {},    0, D_EXIT,       0,   0,    (void*)"Hello World",           NULL, NULL  },*/
/*    { d_button_proc,      28, 260, 160,  24,   {},  {},    0, D_EXIT,       0,   0,    (void*)"Close",                 NULL, NULL  },*/
/*    { d_clock_proc,       10,  10,  200,  200,  {.r = 127},  {},    0,    0,       0,   0,    NULL,          NULL, NULL  },*/
/*    { d_yield_proc,        0,   0,   0,   0,   {},  {},    0,      0,       0,   0,    NULL,                   NULL, NULL  },*/
/*    { NULL,                0,   0,   0,   0,   {},  {},    0,      0,       0,   0,    NULL,                   NULL, NULL  }*/
/*};*/


void run_dialog()
{
  gui_fg_color = al_map_rgb(255, 255, 0);
  gui_mg_color = al_map_rgb(128, 128, 0);
  gui_bg_color = al_map_rgb(0, 0, 63);

  set_dialog_color (the_dialog, gui_fg_color, gui_bg_color);

  the_dialog[1].fg = al_map_rgb(255,255,255);
  the_dialog[1].bg = al_map_rgb(255,0,0);

  do_dialog(the_dialog, 2);
}



