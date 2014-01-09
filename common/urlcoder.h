#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#ifndef _URLCODER_H
#define _URLCODER_H

#define ESCAPE_URI 0
#define ESCAPE_ARGS 1
#define ESCAPE_URI_COMPONENT 2
#define ESCAPE_HTML 3
#define ESCAPE_REFRESH 4
#define ESCAPE_MEMCACHED 5
#define ESCAPE_MAIL_AUTH 6
#define UNESCAPE_URI 1
#define UNESCAPE_REDIRECT 2
#define UNESCAPE_URI_COMPONENT 0

uintptr_t urlencode(u_char *dst, u_char *src, size_t size, unsigned int type);
void urldecode(u_char **dst, u_char **src, size_t size, unsigned int type);

#endif
