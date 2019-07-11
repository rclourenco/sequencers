#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <X11/IntrinsicP.h> // IntrinsicP.h has some faster macros than Intrinsic.h
#include <X11/ShellP.h> // Needed for Widget class definitions 
#include <string.h>
#include "iniparser.h"

#define MAXSONGS 100
int total_songs = 0;

typedef struct {
	char *name;
	char *filename;
} Song;

Song songlist[MAXSONGS];

char *playlist_file = NULL;

int load_songlist(char *filename, Song *list);

XtAppContext app; 
Widget main_widget;

void play_song_external(char *filename);

unsigned long interval = 250;

volatile sig_atomic_t gSignalStatus = 0;
static XtIntervalId    id;

void check_bg (XtPointer client_data, XtIntervalId *id)
{
	Widget list_w = (Widget) client_data;
	if (gSignalStatus) {
		XtAppAddTimeOut (app, (unsigned long) interval, check_bg, (XtPointer) list_w);
		return;
	}
	printf("We are free!\n");
//	XtVaSetValues (list_w, XmNlistEnabled, True, NULL);
	XtSetSensitive (list_w, True);
}

void sel_callback (Widget list_w, XtPointer client_data, XtPointer call_data)
{
	XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data;
	char *choice;

        if (cbs->reason == XmCR_BROWSE_SELECT)
        	printf ("Browse selection -- ");
	else
		printf ("Default action -- ");

	choice = (char *) XmStringUnparse (cbs->item, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
	printf ("selected item: %s (%d)\n", choice, cbs->item_position);
	XtFree (choice);
	if (cbs->item_position > 0 && cbs->item_position <= total_songs) {
		printf("Song Filename: %s\n", songlist[cbs->item_position-1].filename);
		play_song_external(songlist[cbs->item_position-1].filename);
		XtAppAddTimeOut (app, (unsigned long) interval, check_bg, (XtPointer) list_w);
//		XtVaSetValues (list_w, XmNlistEnabled, False, NULL);
		XtSetSensitive (list_w, False);
	}
}

void proc_exit();

unsigned long into = 0;

void catch_events(Widget shell, XtPointer client_data, XEvent *event, Boolean *ctd);

pid_t cpid = -1;

void shutdown() {
	if (gSignalStatus==0)
		return;
	if (cpid>0) {
		kill(cpid, SIGTERM);
	}
	printf("Shutdown!\n");
}

int main (int argc, char **argv)
{
	int status = 0;
	int i, n;
	Widget list, bboard, bboard2;
	XmStringTable    str_list;
	Window w1, w2;
	signal (SIGCHLD, proc_exit);

	atexit(shutdown);

	if (argc>1) {
		playlist_file = argv[1];
		total_songs = load_songlist(playlist_file, songlist);
	}
	n = total_songs;
	if (n==0)
		return 0;

	XtSetLanguageProc (NULL, NULL, NULL);
//	}
        short unsigned int xx,yy;
        //--- Create and initialize the top-level widget 
	main_widget = XtOpenApplication(&app, "MIDI Jukebox", NULL, 0, &argc, argv, NULL, sessionShellWidgetClass, NULL, 0);
	//--- Make it the desired size 
//	XtMakeResizeRequest(main_widget, 400, 100, &xx, &yy);
        
        bboard = XmCreateRowColumn(main_widget, "bboard", NULL, 0);

    /* Set up a translation table that captures "Resize" events 
     *     ** (also called ConfigureNotify or Configure events). If the
     *         ** event is generated, call the function resize().
     *             */
	str_list = (XmStringTable) XtMalloc (n * sizeof (XmString));

	for (i = 0; i < n; i++)
	    str_list[i] = XmStringCreateLocalized (songlist[i].name);

	list = XmCreateScrolledList (bboard, "Hello", NULL, 0);
        XtVaSetValues (list, XmNitems, str_list, XmNitemCount, n, XmNvisibleItemCount, 5, NULL);

	for (i = 0; i < n; i++)
	     XmStringFree (str_list[i]);
	XtFree ((char *) str_list);

        XtManageChild (list);
      
        Arg args[4];	
        n = 0;
        XtSetArg (args[n], XmNwidth, 500);                   n++;
        XtSetArg (args[n], XmNheight, 300);                  n++;
        XtSetArg (args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;    
	bboard2 = XmCreateScrolledWindow(bboard, "player", args, n);
	XtManageChild(bboard2);

	XtManageChild (bboard);
       // XtMakeResizeRequest(bboard2, 400, 400, &xx, &yy);

        XtAddCallback (list, XmNdefaultActionCallback, sel_callback, NULL);
        
	XtAddEventHandler(bboard2, SubstructureNotifyMask|SubstructureRedirectMask, False, catch_events, NULL);

	//--- Realize the main widget 
	XtRealizeWidget(main_widget);
	w1 = XtWindow(main_widget);
	w2 = XtWindow(bboard2);

	printf("Window 1: %lu\n", (unsigned long)w1);
	printf("Window 2: %lu\n", (unsigned long)w2);
	into = (unsigned long)w2;

	XtAppMainLoop(app);
	return status;
}

void catch_events(Widget shell, XtPointer client_data, XEvent *event, Boolean *ctd)
{
	static Window   child_window = 0;
	static Display *child_display = NULL;
	if (event->type == MapRequest) {
		XMapWindow(event->xmaprequest.display, event->xmaprequest.window);
		child_display = event->xmaprequest.display;
		child_window = event->xmaprequest.window;
		printf("Map Request!\n");
	}

	if (event->type == ConfigureNotify || event->type == MapRequest) {
		if (child_window) {
			printf("Configure Notify!\n");

	                // Get container window attributes
			XWindowAttributes attrs;
			XGetWindowAttributes(child_display, XtWindow(shell), &attrs);
			printf("DIM %u  x %u\n", attrs.width, attrs.height);
			XMoveResizeWindow(child_display, child_window, 2, 2, attrs.width - 6, attrs.height - 6);
                }
	}

	if (event->type == DestroyNotify) {
		if (event->xdestroywindow.window == child_window) {
			child_window = 0;
			printf("Child Destroyed!\n");	
		}
	}


}

struct _playlist_loader {
	int pos;
	Song *songs;
};

int playlist_setter(void *data, char *key, char *value);

int playlist_setter(void *data, char *key, char *value)
{
	struct _playlist_loader *sl = (struct _playlist_loader *) data;

	if (key==NULL)
		return 0;

	if (sl->pos>=MAXSONGS)
		return 0;
	if (value==NULL) {
		sl->pos++;
		if (sl->songs[sl->pos].name==NULL)
			sl->songs[sl->pos].name = strdup(key);	
		return 0;
	}

	if (sl->pos<0)
		return 0;

	if (!strcmp(key, "filename")) {
		if (sl->songs[sl->pos].filename==NULL)
			sl->songs[sl->pos].filename = strdup(value);
		return 0;
	}

	return 0;
}

int load_songlist(char *filename, Song *list)
{
    char *ext = rindex(filename, '.');
    if (!ext)
	    return 0;


    if (!strcasecmp(ext, ".slt")) {
		struct _playlist_loader playlist_loader;
		playlist_loader.pos = -1;
		playlist_loader.songs = list;
		ini_parse(filename, &playlist_loader, playlist_setter);
		return playlist_loader.pos+1;
    }
    return 0;
}

void proc_exit()
{
	int wstat;
	pid_t	pid;

	while (TRUE) {
		pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL );
		if (pid == 0)
			return;
		else if (pid == -1)
			return;
		else {
			printf ("Return code: %d\n", wstat);
			gSignalStatus = 0;
		}
	}
}

void play_song_external(char *filename)
{
    int wstatus;

    if (gSignalStatus!=0)
	    return;

    gSignalStatus=1;
    cpid = fork();
    if (cpid == -1) {
       perror("fork");
       gSignalStatus=0;
       return;
    }

    if (cpid == 0) {            /* Code executed by child */
        printf("Child PID is %ld\n", (long) getpid());
	if (into != 0) {
		char tmp[20];
		sprintf(tmp, "%lu", into);
		execlp("xterm","xterm", "-into", tmp ,"-T", "Hello", "-e", "./midibox", filename, NULL);
	}
        _exit(0);
    }
}
