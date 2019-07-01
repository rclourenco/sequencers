#ifndef _INIPARSER_H_

#include <stdio.h>

// if value is NULL is a section
typedef int (*ini_token_func)(void *data, char *key, char *value);

int readline(FILE *fp, char *buffer, int max);
int ini_parse(char *filename, void *data, ini_token_func itf);

#endif
