ALLEGRO_LIBS=`pkg-config --libs allegro-5 allegro_color-5 allegro_font-5 allegro_main-5 allegro_memfile-5 allegro_primitives-5 allegro_acodec-5 allegro_audio-5 allegro_dialog-5 allegro_image-5 allegro_physfs-5 allegro_ttf-5`

bass_sequencer: bass_sequencer.c midi_lib.o
	gcc -o bass_sequencer bass_sequencer.c midi_lib.o -lasound -lm $(ALLEGRO_LIBS) -O2

midi_lib.o: midi_lib/midi_lib.c midi_lib/midi_lib.h
	gcc -c midi_lib/midi_lib.c -o midi_lib.o
