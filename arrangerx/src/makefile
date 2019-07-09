midibox: midibox.c midi_file.c iniparser.c ../../midi_lib.o
	gcc -o midibox midibox.c midi_file.c iniparser.c ../../midi_lib.o -lpthread -lasound -lpanel -lncurses

jukebox: jukebox.c
	gcc `pkg-config --cflags gtk+-3.0` -o jukebox jukebox.c iniparser.c `pkg-config --libs gtk+-3.0`

jukeboxt: jukeboxt.c iniparser.h iniparser.c
	gcc -o jukeboxt jukeboxt.c iniparser.c -lXm -lXt 
