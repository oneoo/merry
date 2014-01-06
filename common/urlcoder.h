#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#ifndef _URLCODER_H
#define _URLCODER_H

uintptr_t urlencode(u_char *dst, u_char *src, size_t size, unsigned int type);
void urldecode(u_char **dst, u_char **src, size_t size, unsigned int type);

#endif
