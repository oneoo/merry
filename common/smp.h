#include <stdlib.h>
#include <string.h>

#ifndef _SMP_H
#define _SMP_H

void *smp_malloc(unsigned int size);
void *smp_realloc(void *p, unsigned int _size);
int smp_free(void *p);

#endif
