#include "urlcoder.h"

size_t urlencode(u_char *dst, u_char *src, size_t size, unsigned int type)
{
    unsigned int n = 0;
    uint32_t *escape;
    u_char *d = dst;
    static u_char hex[] = "0123456789ABCDEF";
    /* " ", "#", "%", "?", %00-%1F, %7F-%FF */
    static uint32_t url[] = {
        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210 /.-, +*)( '&%$ #"! */
        0xfc009fff, /* 1111 1100 0000 0000 1001 1000 0110 1111 */

        /* _^]\ [ZYX WVUT SRQP ONML KJIH GFED CBA@ */
        0x78000001, /* 0111 1000 0000 0000 0000 0000 0000 0001 */

        /* ~}| {zyx wvut srqp onml kjih gfed cba` */
        0xf8000001, /* 1111 1000 0000 0000 0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */
        0xffffffff /* 1111 1111 1111 1111 1111 1111 1111 1111 */
    };

    static uint32_t uri[] = {
        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210 /.-, +*)( '&%$ #"! */
        0xf8000ff5, /* 1111 1000 0000 0000 0000 1000 0110 0101 */

        /* _^]\ [ZYX WVUT SRQP ONML KJIH GFED CBA@ */
        0x78000000, /* 0111 1000 0000 0000 0000 0000 0000 0000 */

        /* ~}| {zyx wvut srqp onml kjih gfed cba` */
        0xf8000001, /* 1111 1000 0000 0000 0000 0000 0000 0001 */

        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */
        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */
        0xffffffff /* 1111 1111 1111 1111 1111 1111 1111 1111 */
    };

    /* " ", "%", %00-%1F */
    static uint32_t memcached[] = {
        0xffffffff, /* 1111 1111 1111 1111 1111 1111 1111 1111 */

        /* ?>=< ;:98 7654 3210 /.-, +*)( '&%$ #"! */
        0x00000021, /* 0000 0000 0000 0000 0000 0000 0010 0001 */

        /* _^]\ [ZYX WVUT SRQP ONML KJIH GFED CBA@ */
        0x00000000, /* 0000 0000 0000 0000 0000 0000 0000 0000 */

        /* ~}| {zyx wvut srqp onml kjih gfed cba` */
        0x00000000, /* 0000 0000 0000 0000 0000 0000 0000 0000 */

        0x00000000, /* 0000 0000 0000 0000 0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000 0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000 0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000 0000 0000 0000 0000 */
    };
    /* mail_auth is the same as memcached */
    static uint32_t *map[] =
    { url, memcached, url, uri };
    escape = map[type];

    if(d == NULL) {
        /* find the number of the characters to be escaped */
        n = 0;

        while(size) {
            if(escape[*src >> 5] & (1 << (*src & 0x1f))) {
                n++;
            }

            src++;
            size--;
        }

        return (uintptr_t) n;
    }

    while(size) {
        if((type == ESCAPE_URL || type == ESCAPE_URI) && *src == ' ') {
            *d++ = '+';
            src++;

        } else if(escape[*src >> 5] & (1 << (*src & 0x1f))) {
            *d++ = '%';
            *d++ = hex[*src >> 4];
            *d++ = hex[*src & 0xf];
            src++;

        } else {
            *d++ = *src++;
        }

        size--;
    }

    return d - dst;
}

size_t urldecode(u_char **dst, u_char **src, size_t size, unsigned int type)
{
    u_char *d, *s, ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;
    d = *dst;
    s = *src;
    state = 0;
    decoded = 0;

    while(size--) {
        ch = *s++;

        switch(state) {
            case sw_usual:
                if(ch == '?'
                   && (type & (UNESCAPE_URI))) {
                    *d++ = ch;
                    goto done;
                }

                if(ch == '%') {
                    state = sw_quoted;
                    break;
                }

                if(ch == '+' && !(type & RAW_UNESCAPE_URL)) {
                    *d++ = ' ';
                    break;
                }

                *d++ = ch;
                break;

            case sw_quoted:
                if(ch >= '0' && ch <= '9') {
                    decoded = (u_char)(ch - '0');
                    state = sw_quoted_second;
                    break;
                }

                c = (u_char)(ch | 0x20);

                if(c >= 'a' && c <= 'f') {
                    decoded = (u_char)(c - 'a' + 10);
                    state = sw_quoted_second;
                    break;
                }

                /* the invalid quoted character */
                state = sw_usual;
                *d++ = ch;
                break;

            case sw_quoted_second:
                state = sw_usual;

                if(ch >= '0' && ch <= '9') {
                    ch = (u_char)((decoded << 4) + ch - '0');
                    *d++ = ch;
                    break;
                }

                c = (u_char)(ch | 0x20);

                if(c >= 'a' && c <= 'f') {
                    ch = (u_char)((decoded << 4) + c - 'a' + 10);

                    if(type & UNESCAPE_URI) {
                        if(ch == '?') {
                            *d++ = ch;
                            goto done;
                        }

                        *d++ = ch;
                        break;
                    }

                    *d++ = ch;
                    break;
                }

                /* the invalid quoted character */
                break;
        }
    }

done:
    return d - *dst;
}
