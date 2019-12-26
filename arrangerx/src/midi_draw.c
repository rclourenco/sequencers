#include <stdio.h>
#include <alloca.h>
#include "midi_pattern.h"

#define MAXTRACKS 2

struct dstatus {
	MidiEventNode *tstart[MAXTRACKS];
	int ntracks;
	int lpattern;
} dstatus;

void draw_events(struct dstatus *status, int tn, unsigned int ticks_quarter, unsigned long ticks)
{
	unsigned int max_tick = ticks_quarter * 4 * 2;
	unsigned long offset = (ticks / max_tick) * max_tick;
	unsigned int grid = ticks_quarter / 8;
	unsigned int steps = max_tick / grid;
	char *table = (char *) alloca(steps);

	if (grid==0)
		grid = 1;

	while (status->tstart[tn] && status->tstart[tn]->time < offset) {
		status->tstart[tn] = status->tstart[tn]->next;
	}

	MidiEventNode *event = status->tstart[tn];
	int step = 0;
	for (unsigned long t = offset + grid; t < offset+max_tick+grid; t+=grid) {
		int has_note = 0;

		while(event && event->time<t) {
			if (event->type == meCV || event->type == meCVRunning) {
				if (event->t.vc.type == 2 && event->datalen==2 && event->data[1] != 0)
				{
					if (event->t.vc.channel==9) {
						switch (event->data[0]) {
								/* Bass Drum */
							case 35:
							case 36:
								has_note |= 2;
								break;
								/* Snare Drum */
							case 37:
							case 38:
							case 39:
							case 40:
								has_note |= 4;
								break;
								/* HitHats */
							case 42:
							case 44:
							case 46:
								has_note |= 8;
								break;
								/* Cymbals */
							case 49:
							case 51:
							case 52:
							case 53:
							case 54:
							case 55:
							case 56:
							case 57:
							case 58:
							case 59:
								has_note |= 16;
								break;
								/* Other */
							default: if (event->data[0]>30)
									 has_note |= 32;
						}
					}
					else
						has_note |= 1;
				}
			}
			event = event->next;
		}

		table[step] = has_note;
		step++;
	}
	printf("\n");

	for (int i=0; i<7; i++)
	{
		if (i==5) {
			printf("\n");
			continue;
		}

		switch(i) {
			case 0: printf("Bass  Drum |"); break;
			case 1: printf("Snare Drum |"); break;
			case 2: printf("HiHat/Ride |"); break;
			case 3: printf("Cymbals    |"); break;
			case 4: printf("OtherDrums |"); break;
			case 6: printf("Bass       |"); break;
		}
		for (int s=0;s<steps;s++)
	       	{
			if (s==steps/2)
			{
				printf("|");
			}

			if (table[s]) {
				char letter = '-';
				switch(i) {
					case 6:
						if (table[s] & 1) letter = 'B';
						break;
					case 0:
						if (table[s] & 2) letter = 'D';
						break;
					case 1:
						if (table[s] & 4) letter = 'S';
						break;
					case 2:
						if (table[s] & 8) letter = 'x';
						break;
					case 3:
						if (table[s] & 16) letter = 'X';
						break;
					case 4:
						if (table[s] & 32) letter = 'T';
				}
				putchar(letter);
			}
			else
				printf("-");

		}
		printf ("|\n");
	}

	printf("           .");
	for (unsigned long t = offset+grid; t < offset +max_tick+grid; t+=grid) {
		if (t>=ticks && t<ticks+grid)
			printf("#");
		else
			printf(" ");
		if ( t % (ticks_quarter * 4) == 0) {
			printf(".");	
		}
	}
	
}

void draw_pattern(MidiPattern **list, int total, int pp, unsigned long ticks)
{
	if (total == 0)
		return;

	if (pp<0 || pp>=total)
		pp = total - 1;

	int reset = 0;
	if (pp!=dstatus.lpattern) {
		reset = 1;
	}

	MidiPattern *pat = list[pp];
	MidiTrackNode *track = pat->tracks;
	int tn = 0;
	dstatus.ntracks = 2;

	printf("\nPlaying: %s [%c%c%c]\n", pat->name, pat->loop ? 'L':'\0', pat->wait ? 'W':'\0', pat->last ? 'E': '\0');
	int i;
	for(i=0;i<MAX_ACTIONS;i++) {
		if (!pat->actions[i])
			continue;
		printf("\tAction %3d: %04X\n", i, pat->actions[i]);
	}

	if (pat->next) {
		printf("\tNext %04X\n", pat->next);
	}
	
	while (track) {
		if (tn > dstatus.ntracks)
			break;
		if (reset || ticks == 0) {
			dstatus.tstart[tn] = track->events;
		}
		printf("\nTrack %2d\n", tn);
		draw_events(&dstatus, tn, pat->ticks_quarter, ticks);
		printf("\n");
		tn++;
		track = track->next;	
	}	
}
