#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "seq.h"


char *LTrim(char *str)
{
	int i=0,j=-1;
	while(str[i])
	{
		if(str[i]!=' ' && str[i]!='\t' && j==-1)
		{
			j=i;
		}
		i++;
	}
	if(j<0) j=i;
	
	memmove(&str[0],&str[j],i-j+1);
	return str;	
}

char *RTrim(char *str)
{
	int i=0,j=-1;
	while(str[i])
	{
		if(str[i]!=' ' && str[i]!='\t')
			j=i;
		i++;
	}
	str[j+1]=0;
	return str;
}

char *Trim(char *str)
{
	return RTrim(LTrim(str));
}


int read_line(FILE *fp, char *buffer, size_t size)
{
	int ch;
	while( (ch=fgetc(fp)) != EOF && ch!='\n' ) {
		
		if(size){
			*buffer=(char)ch;
			buffer++;
			size--;
		}
		
	}
	*buffer='\0';
	return ch;
}

int split(char *string, char by, int num, char *subs[])
{
	int i;
	char *ptr=string;
	subs[0]=ptr;
	for(i=1;i<num;i++)
	{
			ptr=index(ptr,by);
			if(ptr){
				*ptr='\0';
				subs[i]=++ptr;
			}
			else{
				break;
			}
	}
	return i;
}

void note_to_number(char *string, int *note, int *vel, int *l)
{
	int n=0;
	int ofs = 0;
	*vel  = 0;
	*note = 0;
	*l = 0;
	while(*string) {
		if(n==0) {
			switch(*string) {
				case '.': ofs -= 12; break;
				case 'c':
				case 'C': *note = 0; n=1; break;
				case 'd':
				case 'D': *note = 2; n=1; break;
				case 'e':
				case 'E': *note = 4; n=1; break;
				case 'f':
				case 'F': *note = 5; n=1; break;
				case 'g':
				case 'G': *note = 7; n=1; break;
				case 'a':
				case 'A': *note = 9; n=1; break;
				case 'b':
				case 'B': *note = 11; n=1; break;
			}
		}
		else{
			switch(*string) {
				case '.': ofs += 12; break;
				case '#': (*note)++; break;
				case 'b': (*note)--; break; 
				case '_': *l = 1;    break;
			}
		}
		string++;
	}

	*note = (*note) + ofs;
}

void trigger_map(char *string, int *note, int *seq)
{
	int n=0;

	*note = 0;
	
	int sequence = 0;
	while(*string) {
		if(n==0) {
			switch(*string) {
				case 'c':
				case 'C': *note = 0; n=1; break;
				case 'd':
				case 'D': *note = 2; n=1; break;
				case 'e':
				case 'E': *note = 4; n=1; break;
				case 'f':
				case 'F': *note = 5; n=1; break;
				case 'g':
				case 'G': *note = 7; n=1; break;
				case 'a':
				case 'A': *note = 9; n=1; break;
				case 'b':
				case 'B': *note = 11; n=1; break;
			}
		}
		else if(n==1){
			switch(*string) {
				case '#': (*note)++; break;
				case 'b': (*note)--; break; 
				case '=': n=2;       break;
			}
		}
		else if(n==2) {
			if(*string >= '0' && *string<='9') {
				sequence = sequence*10 + (*string)-'0';
			}
			
		}
		string++;
	}

	*seq = sequence;
}


int parse_seqsong(SeqSong *ss, char *var, char *value)
{
	if( !strcasecmp(var,"Name") ) {
		strcpy(ss->song_name,value);
	}
	else if( !strcasecmp(var,"Sequences") ) {
		ss->groups_alloced = atoi(value);
		if(ss->group == NULL && ss->groups_alloced) {
			ss->group = (SeqGroup *)malloc(sizeof(SeqGroup)*ss->groups_alloced);
		}
	}
	else if( !strcasecmp(var,"States") ) {
		ss->states_alloced = atoi(value) ;
		if(ss->state == NULL && ss->states_alloced ) {
			ss->state = (SeqState *)malloc( sizeof(SeqState)*ss->states_alloced );
		}
	}

}

int parse_seqgroup(SeqGroup *sg, char *var, char *value)
{
	int val = 0;
	if( !strcmp(var,"Bpm") ) {
		val = atoi(value);
		val = abs(val) % 800;
		sg->bpm = val;
	}
	else if( !strcmp(var,"Div") ){
		val = atoi(value);
		val = abs(val) % 800;
		sg->div = val;
	}
	else if( !strcmp(var,"Pulse") ){
		val = atoi(value);
		val = abs(val) % 100;
		sg->pulse = val;
	}
	else if( !strcmp(var,"Frag") ){
		val = atoi(value);
		val = abs(val) % MAXCRL;
		sg->frag = val;
	}
	else if( !strcmp(var,"Len") ){
		val = atoi(value);
		val = abs(val) % MAXCRL;
		sg->notes_n = val;
	}

}

int parse_seqstate(SeqState *sstate, char *var, char *value)
{
	int val = 0;
	if( !strcmp(var,"Sequence") ) {
		val = atoi(value);
		val = abs(val) % 800;
		sstate->seq_id = val;
	}
	else if( !strcmp(var,"Next") ){
		val = atoi(value);
		sstate->next = val;
	}
	else if( !strcmp(var,"Triggers") ) {
		int count       = 0;
		char *saveptr1;
		char *token;

		token = strtok_r(value," ",&saveptr1);

		while(token) {
			int note;
			int seq_id;
			trigger_map(token,&note,&seq_id);
			token = strtok_r(NULL," ",&saveptr1);

			sstate->schange[ note%12 ] = seq_id;

			printf("Trigger: %d=>%d\n",note, seq_id);
		}

	}
/*	else if( !strcmp(var,"Pulse") ){*/
/*		val = atoi(value);*/
/*		val = abs(val) % 100;*/
/*		sg->pulse = val;*/
/*	}*/
/*	else if( !strcmp(var,"Frag") ){*/
/*		val = atoi(value);*/
/*		val = abs(val) % MAXCRL;*/
/*		sg->frag = val;*/
/*	}*/
/*	else if( !strcmp(var,"Len") ){*/
/*		val = atoi(value);*/
/*		val = abs(val) % MAXCRL;*/
/*		sg->notes_n = val;*/
/*	}*/

}


int parse_seqchords(SeqChord *sc, char *var, char *value)
{
	int val;
	if( !strcmp(var,"Channel") ) {
		val = atoi(value);
		val = abs(val) % 16;
		sc->channel = val;
	}
	else if( !strcmp(var,"Octave") ) {
		val = atoi(value);
		val = abs(val) % 12;
		sc->octave = val;
	}
	else if( !strcmp(var,"Rhythm") ) {
		int count       = 0;
		char *saveptr1;
		char *token;

		token = strtok_r(value," ",&saveptr1);
		while(token) {
			if(count<12) {
				if(!strcmp(token,"x")) {
					sc->rhythm[count] = 1;
				}
				else if(!strcmp(token,"x_")) {
					sc->rhythm[count] = 2;
				}
				count++;
			}
			
			token = strtok_r(NULL," ",&saveptr1);
		}

		sc->rhythm_len = count;
	}
	
}

int parse_seqchordkeys(SeqChord *sc, char *var, char *value)
{
	int note=0, vel=0, l=0;
	note_to_number(var,&note,&vel,&l);
	int key = abs(note)%12;

	char *token;
	char *saveptr1;

	token = strtok_r(value," ",&saveptr1);
	int count = 0;
	while(token) {
		note_to_number(token,&note,&vel,&l);
		if( count < 4 ) {
			sc->keys[key][count] = abs(note) % 128;
			count++;
		}
		
		token = strtok_r(NULL," ",&saveptr1);
	}
	

}


int parse_seqvoice(SeqVoice *sv, char *var, char *value)
{
	int note = 0;
	int vel = 0;
	int l = 0;

	if( !strcmp(var,"Base") ) {
		note_to_number(value,&note,&vel,&l);
		note = abs(note) % 12;
		sv->base_note = note;
	}
	else if( !strcmp(var,"Octave") ) {
		note = atoi(value);
		note = abs(note) % 12;
		sv->octave = note;
	}
	else if( !strcmp(var,"Vel") ) {
		note = atoi(value);
		note = abs(note) % 128;
		sv->velocity = note;
	}
	else if( !strcmp(var,"Channel") ) {
		note = atoi(value);
		note = abs(note) % 16;
		sv->channel = note;
	}
	else if( !strcmp(var,"Notes") ) {
		int notes_buffer_len = sv->size;
		int note_count       = sv->notes_n;

		char *saveptr1;
		char *token;

		token = strtok_r(value," ",&saveptr1);

		while(token) {
			if(note_count<notes_buffer_len) {
				note_to_number(token,&note,&vel,&l);
				sv->note_a[note_count].k = note;
				sv->note_a[note_count].l = l;
				sv->note_a[note_count].v = vel;
				note_count++;
			}
			
			token = strtok_r(NULL," ",&saveptr1);
		}
		sv->notes_n = note_count;

	}
	else if( !strcmp(var,"Keys") ) {
		sv->key_filtering = 1;
		char *token;
		char *saveptr1;
		token = strtok_r(value," ",&saveptr1);
		while(token) {
			note_to_number(token,&note,&vel,&l);
			note = (note) % 12;
			sv->keys[note] = 1;
			token = strtok_r(NULL," ",&saveptr1);
		}
	}
	return 0;
}

char parse_blocks [10][MAXPARSELINE];

/*
	TODO dinamic note buffer
*/

char buffer[MAXPARSELINE];
char *subs[MAXPARSELINE];


int parse_seq(char *filename, SeqSong *ss)
{
	FILE *fp;
	fp=fopen(filename,"rt");
	if(!fp)
		return -1;
	int mode = 0;
	int block_c = 0;
	int ch;
	int parse_error=0;

	strcpy(ss->song_name,"untitled");
	ss->states_alloced = 0;
	ss->groups_alloced = 0;
	ss->group_count = 0;
	ss->state_count = 0;
	ss->group = NULL;
	ss->state = NULL;

	strcpy(parse_blocks[0],"Song");
	SeqVoice *sv = NULL;
	SeqGroup *sg = NULL;
	SeqChord *sc = NULL;
	SeqState *sstate = NULL;
	do{
		ch = read_line(fp,buffer,255);
		Trim(buffer);
		
		if(buffer[0]!='#' && buffer[0]!='\0') {
			int sp = split(buffer,':',2,subs);
			if(sp>=2) {
				Trim(subs[0]);
				Trim(subs[1]);
				if( !strcmp(subs[1],"{") ) {
					block_c++;
					if(block_c>9){
						break;
						parse_error=1;
					}
					strcpy( parse_blocks[block_c], parse_blocks[block_c-1] );
					strcat(parse_blocks[block_c],".");
					strcat(parse_blocks[block_c],subs[0]);

					if( !strcmp(parse_blocks[block_c],"Song.Group") ) {
						if(ss != NULL && ss->group_count < ss->groups_alloced && ss->group != NULL ) {
							sg = &ss->group[ss->group_count++];
							sg->seq_count = 0;
							sg->notes_n   = 0;
						}
						else
							sg = NULL;
					}
					else if( !strcmp(parse_blocks[block_c],"Song.State") ) {
						if(ss != NULL && ss->state_count < ss->states_alloced && ss->state != NULL ) {
							sstate = &ss->state[ ss->state_count++ ];
							sstate->seq_id = -1;
							sstate->next   = -1;
							int xc;
							for(xc=0;xc<12;xc++) {
								sstate->schange[xc] = -1;
								sstate->keymap[xc] = -1;
							}
						}
						else
							sstate = NULL;
					}
					else if(!strcmp(parse_blocks[block_c],"Song.Group.Seq")) {
						if(sg != NULL && sg->seq_count < MAXSEQS ) {
							sv = &sg->seq[sg->seq_count++];
							sv->notes_n = 0;
							sv->channel = 0;
							sv->base_note = 0;
							sv->octave    = 0;
							sv->velocity  = 0;
							sv->key_filtering = 0;
							sv->size = sg->notes_n;
							sv->note_a = malloc( sizeof(SeqNote)*sv->size );

							int i;
							for(i=0;i<12;i++)
								sv->keys[i] = 0;
						}
						else{
							sv = NULL;
						}
					}
					else if(!strcmp(parse_blocks[block_c],"Song.Group.Chords")) {
						if(sg != NULL && sg->chord_count < MAXCHORDS ) {
							sc = &sg->chord[sg->chord_count++];
							sc->octave     = 0;
							sc->rhythm_len = 0;
							sc->channel    = 0;
							int xc;
							for(xc=0;xc<12;xc++) {
								sc->keys[xc][3] = 
									sc->keys[xc][2] = 
										sc->keys[xc][1] = 
											sc->keys[xc][0] = -1;
							}
						}
						else{
							sc = NULL;
						}
					}

				}
				else{
					if(!strcmp(parse_blocks[block_c],"Song")) {
						if(ss)
							parse_seqsong(ss,subs[0],subs[1]);
					}
					else if(!strcmp(parse_blocks[block_c],"Song.Group")) {
						if(sg)
							parse_seqgroup(sg,subs[0],subs[1]);
					}
					else if(!strcmp(parse_blocks[block_c],"Song.State")) {
						if(sstate)
							parse_seqstate(sstate,subs[0],subs[1]);
					}
					else if(!strcmp(parse_blocks[block_c],"Song.Group.Seq")) {
						if(sv)
							parse_seqvoice(sv,subs[0],subs[1]);
					
					}
					else if(!strcmp(parse_blocks[block_c],"Song.Group.Chords")) {
						if(sc)
							parse_seqchords(sc,subs[0],subs[1]);
					
					}
					else if(!strcmp(parse_blocks[block_c],"Song.Group.Chords.Keys")) {
						if(sc)
							parse_seqchordkeys(sc,subs[0],subs[1]);
					}
					else
						printf("Block %d %s Line %s=%s\n",block_c,parse_blocks[block_c],subs[0],subs[1]);
				}
			
			}
			else if(sp==1) {
				Trim(subs[0]);
				if( !strcmp(subs[0], "{") ) {
					block_c++;
				}
				else if( !strcmp(subs[0],"}") ) {
					if(block_c>0) {
						block_c--;
					}
					else{
						parse_error = 1;
						break;
					}
				}
			}
		}
	}while(ch!=EOF);
	fclose(fp);
}

SeqSong ss;
SeqGroup sg;

int dump_song(SeqSong *ss)
{

	int j,c;
	printf("Song:  %s\n",ss->song_name);
	for(c=0;c<ss->group_count; c++ ) {
		SeqGroup *sg = &ss->group[c];
		printf("Group\n");
		printf("\tbpm:   %d\n", sg->bpm);
		printf("\tdiv:   %d\n", sg->div);
		printf("\tpulse: %d\n", sg->pulse);
		printf("\tfrag:  %d\n", sg->frag);
		for(j=0;j<sg->seq_count; j++) {
			printf("\tSeq\n");
			SeqVoice *sv = &sg->seq[j];
			if(sv->note_a != NULL ){
				printf("\t\tChannel   %d\n", sv->channel);
				printf("\t\tBase note %d\n", sv->base_note);
				printf("\t\tOctave    %d\n", sv->octave);
				printf("\t\tVelocity  %d\n", sv->velocity);
				printf("\t\tSeq len   %d\n", sv->notes_n);
				printf("\t\tNotes: ");
				int i;
				for( i=0; i< sv->notes_n; i++ )
				{
					SeqNote *sn = &sv->note_a[i];
					printf("v%d k%d %c |",sn->v, sn->k,sn->l ? 'l': ' ');
					if(i%8 == 7){
						printf("\n\t\t\t");
					}
				}
				printf("\n");
				if(sv->key_filtering) {
					printf("\t\tKeys: ");
					for(i=0;i<12;i++) {
						if(sv->keys[i])
							printf("%d ",i);
					}
					printf("\n");
				}
			}
		}
		for(j=0;j<sg->chord_count; j++) {
			printf("\tChord\n");
			SeqChord *sc = &sg->chord[j];
			printf("\t\tChannel: %d\n",sc->channel);
			printf("\t\tOctave: %d\n",sc->octave);
			int i;
			printf("\t\tRhythm: ");
			for(i=0;i < sc->rhythm_len; i++) {
				printf("%d ",sc->rhythm[i]);
			}
			printf("\n");
			printf("\t\tKeys\n");
			for(i=0;i<12;i++) {
				if( sc->keys[i][0] != -1 ){
					printf("\t\t\t%d: %d %d %d %d\n", i,sc->keys[i][0], sc->keys[i][1], sc->keys[i][2], sc->keys[i][3]);
					
				}
			}
			printf("\n");
		}
	}
	return 0;
}

