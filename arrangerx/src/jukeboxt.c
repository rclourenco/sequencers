#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/FileSB.h>
#include <X11/IntrinsicP.h> // IntrinsicP.h has some faster macros than Intrinsic.h
#include <X11/ShellP.h> // Needed for Widget class definitions 
#include <string.h>
#include "iniparser.h"
#include "../../midi_lib/midi_lib.h"
#include <X11/bitmaps/mailfull>


#define MAXSONGS 100
int total_songs = 0;

typedef struct {
	char *name;
	char *filename;
} Song;

Song songlist[MAXSONGS];
char *playlist_folder = NULL;
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

void pushed(Widget w, XtPointer client_data, XtPointer call_data);

Widget list;

void update_list(Widget list, char *filename)
{
	int i;
	for (i=0;i<total_songs;i++) {
		if (songlist[i].name)
			free(songlist[i].name);
		if (songlist[i].filename)
			free(songlist[i].filename);
		songlist[i].name = NULL;
		songlist[i].filename = NULL;
	}
	total_songs = 0;
	total_songs = load_songlist(filename, songlist);
	XmListDeleteAllItems (list);
	if (total_songs == 0)
		return;

	XmStringTable str_list = (XmStringTable) XtMalloc (total_songs * sizeof (XmString));

	for (i = 0; i < total_songs; i++) {
	    printf("Adding: %s\n", songlist[i].name);
	    str_list[i] = XmStringCreateLocalized (songlist[i].name);
	}

	XmListAddItemsUnselected (list, str_list, total_songs, 1);

	for (i = 0; i < total_songs; i++) {
		XmStringFree(str_list[i]);
	}
	XtFree((char *)str_list);


}

void fileselector_action(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *filename;
	XmFileSelectionBoxCallbackStruct *cbs =
		(XmFileSelectionBoxCallbackStruct *) call_data;
	filename = (char *) XmStringUnparse (cbs->value,
			XmFONTLIST_DEFAULT_TAG,
			XmCHARSET_TEXT,
			XmCHARSET_TEXT,
			NULL, 0, XmOUTPUT_ALL);
	if (!filename)
		/* must have been an internal error */
		return;

	switch (cbs->reason) {
		case XmCR_OK: 
			if (*filename) {
				update_list(list, filename);
			}
			break;
	}

	XtFree (filename);
	XtDestroyWidget (XtParent (w));

}

void my_callback (Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget dialog;
	Arg args[2];

	XmPushButtonCallbackStruct *cbs =
		(XmPushButtonCallbackStruct *) call_data;
        XmString title = XmStringCreateLocalized("Open playlist...");

	XtSetArg (args[0], XmNpathMode, XmPATH_MODE_RELATIVE);
	XtSetArg (args[1], XmNdialogTitle, title);
	dialog = XmCreateFileSelectionDialog (w, "filesb", args,2);
	XtAddCallback (dialog, XmNcancelCallback, fileselector_action, NULL);
	XtAddCallback (dialog, XmNokCallback, fileselector_action, NULL);
	XmStringFree(title);
	XtManageChild (dialog);
}

Widget PostDialog();

typedef struct {
	char *name;
	char *label;
	char **strings;
	int size;
	int current;
	void (*on_update) (char *value, void *data);
	void *cb_data;
} ListItem;

#define MAXITEMS 30

void listItemFlush(ListItem *litems)
{
	int i;

	if (litems->strings==NULL) {
		litems->size=0;
		litems->current=0;
		return;
	}

	for(i=0;i<litems->size;i++) {
		free(litems->strings[i]);
	}

	litems->size=0;
	litems->current=0;
}

void listItemClear(ListItem *litems)
{
	listItemFlush(litems);

	free(litems->strings);
	litems->strings = NULL;
}


int listItemSet(ListItem *litems, char *str)
{
	int i;
	if (litems->strings==NULL) {
		litems->strings = (char **)malloc(sizeof(char *)*MAXITEMS);
		if (litems->strings==NULL)
			return -2;
		litems->size=0;
		litems->current=0;
	}

	i = litems->size;
	if (i>=MAXITEMS)
		return -1;
	litems->strings[i] = strdup(str);

	if (litems->strings[i]==NULL)
		return -2;

	litems->size++;
	return litems->size;
}


ListItem midiin_list  = {"midiin", "Midi Input Devices", NULL, 0, 0, NULL, NULL};
ListItem midiout_list = {"midiout", "Midi Output Devices", NULL, 0, 0, NULL, NULL};


char *getMidiIn()
{
	if (midiin_list.current >=0 && midiin_list.current < midiin_list.size) {
		return midiin_list.strings[midiin_list.current];
	}
	return NULL;
}

char *getMidiOut()
{
	if (midiout_list.current >=0 && midiout_list.current < midiout_list.size) {
		return midiout_list.strings[midiout_list.current];
	}
	return NULL;
}

void buildOptionsPanel(Widget parent)
{
	XmString btn_text;
	Widget button;

	btn_text = XmStringCreateLocalized ("Open Playlist");
        button = XtVaCreateWidget ("select_playlist", xmPushButtonWidgetClass, parent,
							XmNlabelString, btn_text, NULL);
	XmStringFree (btn_text);
	XtAddCallback(button, XmNactivateCallback, my_callback,(XtPointer) NULL);
	XtManageChild(button);

	btn_text = XmStringCreateLocalized ("Midi In");
        button = XtVaCreateWidget ("select_midi_in", xmPushButtonWidgetClass, parent,
							XmNlabelString, btn_text, NULL);
	XmStringFree (btn_text);
	XtAddCallback(button, XmNactivateCallback, pushed, (XtPointer) &midiin_list);
	XtManageChild(button);

	btn_text = XmStringCreateLocalized ("Midi Out");
        button = XtVaCreateWidget ("select_midi_out", xmPushButtonWidgetClass, parent,
							XmNlabelString, btn_text, NULL);
	XmStringFree (btn_text);
	XtAddCallback(button, XmNactivateCallback, pushed, (XtPointer) &midiout_list);
	XtManageChild(button);
}

void SetIconWindow (Widget shell, Pixmap image);

char *exec_dir = NULL;
char *midibox = "midibox";

int main (int argc, char **argv)
{
	int status = 0;
	int i, n;
	Widget bboard, bboard1, bboard2;
	XmStringTable    str_list;
	Window w1, w2;
	signal (SIGCHLD, proc_exit);

	atexit(shutdown);

	if (argc>0) {
		char *p = strdup(argv[0]);
		if (p) {
			exec_dir = dirname(p);
			if (exec_dir) {
				exec_dir = strdup(exec_dir);
			}
			free(p);
		}
	}

	if (argc>1) {
		playlist_file = argv[1];

		printf("Total Songs: %u\n", total_songs);

		total_songs = load_songlist(playlist_file, songlist);
	}
	n = total_songs;

	printf("Total Songs: %u\n", total_songs);
/*	if (n==0)
		return 0; */

	XtSetLanguageProc (NULL, NULL, NULL);
//	}
        short unsigned int xx,yy;
        //--- Create and initialize the top-level widget 
	main_widget = XtVaOpenApplication(&app, 
			"MIDI Jukebox",
		       	NULL, 0,
		       	&argc, argv, NULL,
		       	sessionShellWidgetClass,
			XmNiconic, False,
		       	NULL);
        int depth = DefaultDepthOfScreen(XtScreen(main_widget));
	printf("Depth: %d\n", depth);
        Pixmap bitmap = XCreatePixmapFromBitmapData(XtDisplay(main_widget),
		RootWindowOfScreen (XtScreen (main_widget)),
		(char *) mailfull_bits,
		mailfull_width,
		mailfull_height,
		0xFFFFFF, 0, depth);

	XtVaSetValues (main_widget, XmNiconPixmap, bitmap, NULL);
	SetIconWindow (main_widget, bitmap);

	//--- Make it the desired size 
//	XtMakeResizeRequest(main_widget, 400, 100, &xx, &yy);
        
        bboard = XmCreateRowColumn(main_widget, "bboard", NULL, 0);
        bboard1 = XmCreateRowColumn(bboard, "tools", NULL, 0);

    /* Set up a translation table that captures "Resize" events 
     *     ** (also called ConfigureNotify or Configure events). If the
     *         ** event is generated, call the function resize().
     *             */
	str_list = NULL;
	if (n>0) {
		str_list = (XmStringTable) XtMalloc (n * sizeof (XmString));

		for (i = 0; i < n; i++)
	    		str_list[i] = XmStringCreateLocalized (songlist[i].name);
	}

	list = XmCreateScrolledList (bboard1, "Hello", NULL, 0);
        XtVaSetValues (list, XmNitems, str_list, XmNitemCount, n, XmNvisibleItemCount, 5, NULL);

	if (str_list != 0) {
		for (i = 0; i < n; i++)
	     		XmStringFree (str_list[i]);
		XtFree ((char *) str_list);
	}

        XtManageChild (list);
      
        Arg args[4];	
        n = 0;
        XtSetArg (args[n], XmNwidth,  800);                  n++;
        XtSetArg (args[n], XmNheight, 600);                  n++;
        XtSetArg (args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;    
	bboard2 = XmCreateScrolledWindow(bboard, "player", args, n);
	XtManageChild(bboard2);

	buildOptionsPanel(bboard1);

	XtManageChild(bboard1);
	XtManageChild(bboard);
        //XtMakeResizeRequest(bboard2, 400, 400, &xx, &yy);


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

void SetIconWindow (Widget shell, Pixmap image)
{
	Window
		window, root;
	unsigned int
		width, height, border_width, depth;
	int
		x, y;
	Display
		*dpy = XtDisplay (shell);
	/* Get the current icon window associated with the shell */
	XtVaGetValues (shell, XmNiconWindow, &window, NULL);
	if (!window) {
		/* If there is no window associated with the shell, create one.
		 * ** Make it at least as big as the pixmap we're
		 * ** going to use. The icon window only needs to be a simple window.
		 * */
		if (!XGetGeometry (dpy, image, &root, &x, &y, &width, &height,
					&border_width, &depth) ||
				!(window = XCreateSimpleWindow (dpy, root, 0, 0,
						width, height, (unsigned)0, CopyFromParent,
						CopyFromParent)))
		{
			XtVaSetValues (shell, XmNiconPixmap, image, NULL);
			return;
		}
		/* Now that the window is created, set it... */
		XtVaSetValues (shell, XmNiconWindow, window, NULL);
	}
	/* Set the window's background pixmap to be the image. */
	XSetWindowBackgroundPixmap (dpy, window, image);
	/* cause a redisplay of this window, if exposed */
	XClearWindow (dpy, window);
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

		if (playlist_folder)
			free(playlist_folder);
		playlist_folder = NULL;

		char *tmp = strdup(filename);
		if (tmp) {
			char *t2 = dirname(tmp);
			if (t2)
				playlist_folder = strdup(t2);
			free(tmp);
		}

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

char *extract_mididev(char *option)
{
	char *r = strrchr(option, '[');
	if (!r)
		return NULL;
	r++;
	r = strdup(r);
	char *p = strchr(r, ']');
	if (p!=NULL)
		*p = '\0';
	return r;
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

		char *path = filename;
		int free_path = 0;
		if (playlist_folder && *playlist_folder) {
			path = (char *)malloc(strlen(playlist_folder) + strlen(filename) + 2);
			strcpy(path, playlist_folder);
			strcat(path, "/");
			strcat(path, filename);
			free_path = 1;
		}

		char *midiin = getMidiIn();
		char *midiout = getMidiOut();

		char *midiin_a = NULL;
		char *midiout_a = NULL;

		char *mpath = exec_dir ? exec_dir : ".";
		size_t l = strlen(mpath)+1+strlen(midibox)+1;
		char *mbox = (char *) malloc(l);
		strcpy(mbox, mpath);
		strcat(mbox, "/");
		strcat(mbox, midibox);
		if (!mbox)
			_exit(2);
		printf("L: %u %u => %s\n", l, strlen(mbox), mbox);

		char *args[20];
		int ac = 0;
		args[ac++] = "xterm";
		args[ac++] = "-into";
		args[ac++] = tmp;
                args[ac++] = "-T";
                args[ac++] = "MidiBox";
                args[ac++] = "-e";
		args[ac++] = mbox;

		if (midiin) {
			midiin_a = extract_mididev(midiin);
			if (midiin_a) {
				args[ac++] = "--in";
				args[ac++] = midiin_a;
			}
		}

		if (midiout) {
			midiout_a = extract_mididev(midiout);
			if (midiout_a) {
				args[ac++] = "--out";
				args[ac++] = midiout_a;
			}
		}

		args[ac++] = path;
		args[ac] = NULL;
		
		execvp("xterm", args);
		if (free_path)
			free(path);
		if (midiin_a)
			free(midiin_a);
		if (midiout_a)
			free(midiin_a);
	}
        _exit(0);
    }
}

/*******************************************************************************/
/******************************************************************************/

void selectionDialog(Widget widget, ListItem *items);
/* pushed() --the callback routine for the main app's pushbutton.
 * ** Create a dialog containing the list in the items parameter.
 * */
void pushed (Widget widget, XtPointer client_data, XtPointer call_data)
{
	ListItem *items = (ListItem *) client_data;
	int l_cur = items->current;

	DeviceEntry *mdevice = midi_get_devices();
	listItemFlush(items);

	while (mdevice) {
		char tmp[100];
		sprintf(tmp, "%s [%s]", mdevice->name, mdevice->device);
		listItemSet(items, tmp);
		mdevice = mdevice->next;
	}
	
	midi_free_device_list(mdevice);
	if (l_cur < items->size)
		items->current = l_cur;
	
	selectionDialog(widget, items);
}

void selectionDialog(Widget widget, ListItem *items)
{
	Widget dialog;
	XmString t, title, *str = NULL;
	int i;
	extern void dialog_callback();
	if (items->size>0)
		str = (XmString *) XtMalloc (items->size * sizeof (XmString));
	t = XmStringCreateLocalized (items->label);
        title = XmStringCreateLocalized("Select an item");

	for (i = 0; i < items->size; i++)
		str[i] = XmStringCreateLocalized (items->strings[i]);
	dialog = XmCreateSelectionDialog (widget, "selection", NULL, 0);
	XtVaSetValues (dialog,
			XmNlistLabelString, t,
			XmNlistItems, str,
			XmNlistItemCount, items->size,
			XmNmustMatch, True,
			XmNdialogTitle, title,
			NULL);

//	XtVaSetValues (XtParent(dialog), XmNtitle, "Fuck!", NULL); 

	XtSetSensitive (XtNameToWidget (dialog, "Help"), False);

	if (items->current >= 0 && items->current < items->size) {
		Widget sel = XtNameToWidget(dialog, "*ItemsList");
		XmListSelectPos(sel, items->current+1, True);
	}

	XtAddCallback (dialog, XmNokCallback, dialog_callback, items);
	XtAddCallback (dialog, XmNnoMatchCallback, dialog_callback, items);
	XmStringFree (t);
	XmStringFree (title);
	if (str) {
		while (--i >= 0)
			XmStringFree (str[i]); /* free elements of array */
		XtFree ((char *) str); /* now free array pointer */
	}
	XtManageChild (dialog);
}

/* dialog_callback() --The OK button was selected or the user
 * ** input a name by himself. Determine whether the result is
 * ** a valid name by looking at the "reason" field.
 * */
void dialog_callback (Widget widget, XtPointer client_data,
		XtPointer call_data)
{
	ListItem *items = (ListItem *) client_data;

	char
		msg[256], *prompt, *value;
	int
		dialog_type;
	XmSelectionBoxCallbackStruct *cbs =
		(XmSelectionBoxCallbackStruct *) call_data;

	int newop = -1;
	int i;
	int ok = 0;

	switch (cbs->reason) {
		case XmCR_OK: prompt = "Selection: ";
				dialog_type = XmDIALOG_MESSAGE;
			      ok=1;
			break;
		case XmCR_NO_MATCH: prompt = "Not a valid selection: ";
			     dialog_type = XmDIALOG_ERROR;
			break;
		default: prompt = "Unknown selection: ";
			     dialog_type = XmDIALOG_ERROR;
			     break;
	}

	value = (char *) XmStringUnparse (cbs->value, XmFONTLIST_DEFAULT_TAG, XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);

	if (ok) {
		for(i=0;i<items->size;i++) {
			if (!strcmp(value, items->strings[i])) {
				newop = i;
				break;
			}
		}
		items->current = newop;
	}	
	sprintf (msg, "%s%s:%d", prompt, value, newop);
	XtFree (value);
	(void) PostDialog (XtParent (XtParent (widget)), dialog_type, msg);
	if (cbs->reason != XmCR_NO_MATCH) {
		XtUnmanageChild (widget);
		/* The shell parent of the Selection box */
		XtDestroyWidget (XtParent (widget));
	}
}

/*
 * ** Destroy the shell parent of the Message box, and thus the box itself
 * */
void destroy_dialog (Widget dialog, XtPointer client_data, XtPointer call_data)
{
	XtDestroyWidget (XtParent (dialog));
	/* The shell parent of the Message box */
}
/*
 * ** PostDialog() -- a generalized routine that allows the programmer
 * ** to specify a dialog type (message, information, error, help,
 * ** etc..), and the message to show.
 * */
Widget PostDialog (Widget parent, int dialog_type, char *msg)
{
	Widget dialog;
	XmString text;
	dialog = XmCreateMessageDialog (parent, "dialog", NULL, 0);
	text = XmStringCreateLocalized (msg);
	XtVaSetValues (dialog, XmNdialogType, dialog_type,
			XmNmessageString, text, NULL);
	XmStringFree (text);
	XtUnmanageChild (XtNameToWidget (dialog, "Cancel"));
	XtSetSensitive (XtNameToWidget (dialog, "Help"), False);
	XtAddCallback (dialog, XmNokCallback, destroy_dialog, NULL);
	XtManageChild (dialog);
	return dialog;
}
