#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#ifndef _URLCODER_H
#define _URLCODER_H

#ifndef u_char
typedef unsigned char u_char;
#endif

#define ESCAPE_URL 0
#define ESCAPE_MEMCACHED 1
#define RAW_ESCAPE_URL 2
#define ESCAPE_URI 3
#define UNESCAPE_URL 0
#define RAW_UNESCAPE_URL 1
#define UNESCAPE_URI 2

size_t urlencode(u_char *dst, u_char *src, size_t size, unsigned int type);
size_t urldecode(u_char **dst, u_char **src, size_t size, unsigned int type);

#endif
