#include <gtk/gtk.h>
#include <string.h>
#include "iniparser.h"

#define MAXSONGS 100
int total_songs = 0;

typedef struct {
	char *name;
	char *filename;
} Song;

Song songlist[MAXSONGS];

static void print_hello (GtkWidget *widget, gpointer data)
{
	g_print ("Hello World\n");
}

char *songs[] = {
	"abcd",
	"asdf",
	"asdr",
	"wqer",
	NULL
};

char *playlist_file = NULL;

int load_songlist(char *filename, Song *list);


static void list_activate(GtkListBox  *box, GtkListBoxRow *row, gpointer user_data) {
        GError *err = NULL;
	g_print("Click!\n");
	gint i = gtk_list_box_row_get_index (row);
	if (i>=0 && i <total_songs) {
		g_print(">>>> %s\n", songlist[i].filename);
		char tmp[200] = "xterm "; 
		strcat(tmp, " -T \"Playing: ");
		strcat(tmp, songlist[i].name);
		strcat(tmp, "\" -e \"./midibox ");
		strcat(tmp, songlist[i].filename);
		strcat(tmp, "\"");
		  if (!g_spawn_command_line_sync (tmp,NULL, NULL, NULL, &err))
	          {
			g_error_free (err);
		  }

	}
}


static void open_files (GtkApplication *app, GFile **files, gint n_files, const gchar *hint)
{

//	printf("Playlist: %s\n", playlist_file);	
	if (playlist_file==NULL)
		return;
//	printf("Number of files %d\n", n_files);
	int i;
	for(i=0;i<n_files;i++) {
		gchar *x=g_file_get_path(files[i]); 
		g_print("File: %s\n", x);
		total_songs = load_songlist(x, songlist);

		g_free(x);
		break;
	}
//	printf("Total Songs: %u\n", total_songs);

	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *button_box;

//	if (playlist_file) 
//	  total_songs = load_songlist(playlist_file, songlist);
	g_print("Total Songs: %u\n", total_songs);

	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "Jukebox");
	gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

	button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_container_add (GTK_CONTAINER (window), button_box);

	button = gtk_button_new_with_label ("Hello World");
	g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
	gtk_container_add (GTK_CONTAINER (button_box), button);


	GtkWidget *list = gtk_list_box_new ();
	
	for(i=0; i<total_songs; i++)
       	{
		GtkWidget *row = gtk_label_new (songlist[i].name);
		gtk_container_add (GTK_CONTAINER(list), row);
	}
	g_signal_connect (list, "row-activated", G_CALLBACK (list_activate), NULL);

	gtk_container_add(GTK_CONTAINER (button_box), list);

	gtk_widget_show_all (window);

}
	
static void activate (GtkApplication *app, gpointer user_data)
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *button_box;

//	if (playlist_file) 
//	  total_songs = load_songlist(playlist_file, songlist);
	g_print("Total Songs: %u\n", total_songs);

	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "Jukebox");
	gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);



	button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_container_add (GTK_CONTAINER (window), button_box);

	button = gtk_button_new_with_label ("Hello World");
	g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
	gtk_container_add (GTK_CONTAINER (button_box), button);

	GtkWidget *list = gtk_list_box_new ();
	
	int i=0;
	for(i=0; i<total_songs; i++)
       	{
		gtk_container_add (GTK_CONTAINER(list), gtk_label_new (songlist[i].name));
	}
	g_signal_connect (list, "row-selected", G_CALLBACK (list_activate), NULL);

	gtk_container_add(GTK_CONTAINER (button_box), list);

	gtk_widget_show_all (window);
}

int main (int argc, char **argv)
{
	GtkApplication *app;
	int status;

//if (argc>1) {
		playlist_file = "playlist.slt";
//	}
	app = gtk_application_new ("org.gtk.example", G_APPLICATION_HANDLES_OPEN);
	g_signal_connect (app, "open", G_CALLBACK (open_files), NULL);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);

	return status;
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
