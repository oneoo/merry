#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _SMP_H
#define _SMP_H

void *smp_malloc(unsigned int size);
void *smp_realloc(void *p, unsigned int _size);
int smp_free(void *p);

void *_smp_malloc(unsigned int size, char *f, int l);
void *_smp_realloc(void *p, unsigned int size, char *f, int l);
int _smp_free(void *p, char *f, int l);

void dump_smp_link();

#else

#ifdef SMPDEBUG
#define malloc(s) _smp_malloc(s, __FILE__, __LINE__)
#define realloc(p,s) _smp_realloc(p,s, __FILE__, __LINE__)
#define free(p) _smp_free(p, __FILE__, __LINE__)
#else
#define malloc(s) smp_malloc(s)
#define realloc(p,s) smp_realloc(p,s)
#define free(p) smp_free(p)
#endif

#endif
