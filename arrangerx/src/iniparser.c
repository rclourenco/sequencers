#include <stdio.h>
#include "iniparser.h"

int ini_parse(char *filename, void *data, ini_token_func itf)
{
	char buffer[80];
	FILE *fp = fopen(filename, "rt");
	if (!fp)
		return -2;
	while(!feof(fp)) {
		if (!readline(fp, buffer, 80)) {
			continue;
		}
		int i=0;

		while(buffer[i]==' ') i++;
		if (buffer[i]=='#')
			continue;
		if (buffer[i]=='[') {
			i++;
			while(buffer[i]==' ') i++;
			int sf = i;
			while(buffer[sf] && buffer[sf]!=']') sf++;
			while(sf>i && buffer[sf-1]==' ') sf--;
			buffer[sf]='\0';
			char *section = &buffer[i];
			if (itf)
				itf(data, section, NULL);
			continue;
		}
		if (!buffer[i])
			continue;

		int kf = i;
		while(buffer[kf]!='=' && buffer[kf]) kf++;
		if (!buffer[kf]) {
//			printf("Error @ %s\n", buffer);
			continue;
		}
		int vi = kf+1;

		while(kf>0 && buffer[kf-1]==' ') kf--;
		if (kf == 0) {
//			printf("Error @ %s\n", buffer);
			continue;
		}

		while(buffer[vi]==' ') vi++;

		int vf = vi;
		while(buffer[vf]) vf++;

		while(vf>vi && buffer[vf-1]==' ') vf--;

		buffer[kf] = '\0';
		buffer[vf] = '\0';
		char *key = &buffer[i];
		char *value = &buffer[vi];

		if (itf)
			itf(data, key, value);
	}
	fclose(fp);
}

int readline(FILE *fp, char *buffer, int max)
{
	int l=0;
	char ch;
	if (max<1)
		return 0;
	max--;
        while( (ch=fgetc(fp)) != '\n' && ch != EOF ) {
          if(l<max) buffer[l++] = ch;
        }
	buffer[l]='\0';
	return l;
}
