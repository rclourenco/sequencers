#ALLEGRO_LIBS=`pkg-config --libs allegro-5 allegro_color-5 allegro_font-5 allegro_main-5 allegro_memfile-5 allegro_primitives-5 allegro_acodec-5 allegro_audio-5 allegro_dialog-5 allegro_image-5 allegro_physfs-5 allegro_ttf-5`

#gcc -o sequencer seq.c parse_seq.c gui.c midiinput.c -lpthread -lasound `allegro-config --libs`


gcc -o sequencer seq.c parse_seq.c midiinput.c -lpthread -lasound
