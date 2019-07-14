midi_lib.o: midi_lib.c midi_lib.h
	gcc -c midi_lib.c -o midi_lib.o

test: midi_lib.c midi_lib.h
	gcc midi_lib.c -o midi_lib_test -Dmake_test=1 -lasound
