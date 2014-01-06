#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/timeb.h>

#ifndef _STRINGS_H
#define _STRINGS_H

int stricmp(const char *str1, const char *str2);
char *stristr(const char *str, const char *pat, int length);
void random_string(char *string, size_t length, int s);

#endif
