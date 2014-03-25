#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/timeb.h>
#include <math.h>

#ifndef _STRINGS_H
#define _STRINGS_H

int stricmp(const void *s1, const void *s2);
int strincmp(const void *s1, const void *s2, size_t n);
const char *stristr(const void *str, const void *pat, int length);
void random_string(void *string, size_t length, int s);

unsigned long _strtoul(void *str64, int base);
char *_ultostr(void *str, unsigned long val, unsigned base);
char *strsplit(const void *string_org, int org_len, const char *demial, char **last, int *len);

#endif
