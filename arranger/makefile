ALLEGRO_LIBS=`pkg-config --libs allegro-5 allegro_color-5 allegro_font-5 allegro_main-5 allegro_memfile-5 allegro_primitives-5 allegro_acodec-5 allegro_audio-5 allegro_dialog-5 allegro_image-5 allegro_physfs-5 allegro_ttf-5`

INCLUDES="-I../../gui"
#gluxtest: gluxtest.c glux.o gluxproc.o glux.h
#	gcc -o gluxtest gluxtest.c gluxproc.o glux.o -lm $(ALLEGRO_LIBS) -O2
OBJS=../../gui/glux.o ../../gui/gluxproc.o

sequencer:
	gcc -o sequencer $(OBJS) seq.c parse_seq.c midiinput.c seqgui.c $(INCLUDES) -lpthread -lasound -lm $(ALLEGRO_LIBS) -O2

