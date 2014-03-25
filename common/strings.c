#include "strings.h"

int stricmp(const void *s1, const void *s2)
{
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;
    int result;

    if(p1 == p2) {
        return 0;
    }

    while((result = tolower(*p1) - tolower(*p2++)) == 0) {
        if(*p1++ == '\0') {
            break;
        }
    }

    return result;
}

int strincmp(const void *s1, const void *s2, size_t n)
{
    const char *p1 = s1;
    const char *p2 = s2;
    int d = 0;

    for(; n != 0; n--) {
        int c1 = tolower(*p1++);
        int c2 = tolower(*p2++);

        if(((d = c1 - c2) != 0) || (c2 == '\0')) {
            break;
        }
    }

    return d;
}

const char *stristr(const void *str, const void *pat, int length)
{
    const char *_str = str;
    const char *_pat = pat;
    int _dict[128] = {0};

    if(!_str || !_pat) {
        return NULL;
    }

    if(length < 1) {
        length = strlen(_str);
    }

    int pat_len = 0, i = 0, j = 0, m = 0;

    while(_pat[i++]) {
        _dict[tolower(_pat[pat_len]) % 128] = pat_len + 1;
        pat_len++;
    }

    for(i = pat_len - 1; i < length; i += pat_len) {
        j = _dict[tolower(_str[i]) % 128];

        if(j) {
            m = i;
            i -= (j - 1);

            if(tolower(_str[i]) != tolower(_pat[0])) {
                continue;
            }

            for(; i < m + (pat_len - j); i++) {
                if((length - i) >= pat_len && tolower(_str[i + pat_len - 1]) == tolower(_pat[pat_len - 1])) {
                    for(j = 1; j < pat_len; j++) {
                        if(tolower(_str[i + j]) != tolower(_pat[j])) {
                            break;
                        }
                    }

                    if(j < pat_len) {
                        continue;
                    }

                    return (char *)_str + i;
                }
            }
        }
    }

    return NULL;
}

void random_string(void *string, size_t length, int s)
{
    char *_string = (char *)string;
    /* Seed number for rand() */
    struct timeb t;
    ftime(&t);
    srand((unsigned int) 1000 * t.time + t.millitm + s);

    unsigned int num_chars = length;
    unsigned int i;
    unsigned int j = rand();

    for(i = 0; i < num_chars; ++i) {
        if(j % 1000 < 500) {
            _string[i] = j % ('g' - 'a') + 'a';

        } else {
            _string[i] = j % (':' - '0') + '0';
        }

        j = rand();
    }
}

unsigned long _strtoul(void *str64, int base)
{
    char *_str64 = (char *)str64;
    unsigned long i, j, nResult = 0;
    char _t[32] = {0};
    int m = strlen(_str64);

    if(m > 32) {
        return 0;
    }

    for(i = 0; i < m; i++) {
        if(_str64[i] == '\r' || _str64[i] == '\n' || _str64[i] == '\t' || _str64[i] == ' ' || _str64[i] == ';'
           || _str64[i] == '-') {
            break;
        }

        _t[i] = _str64[i];
    }

    for(i = 0; i < strlen(_t); i++) {
        j = _t[i] == ',' ? 62 : (_t[i] == '.' ? 63 : (_t[i] <= '9' ? _t[i] - '0' :
                                 (_t[i] <= 'Z' ? 36 + _t[i] - 'A' : 10 + _t[i] - 'a')));
        nResult += pow(base, (strlen(_t) - i - 1)) * j ;
    }

    return nResult;
}

typedef struct {
    unsigned long quot;
    unsigned long rem;
} _uldiv_t;

static _uldiv_t _uldiv(unsigned long number, unsigned long denom)
{
    _uldiv_t rv;

    rv.quot = number / denom;
    rv.rem = number % denom;
    return rv;
}

char *_ultostr(void *str, unsigned long val, unsigned base)
{
    char *_str = (char *)str;
    _uldiv_t r;

    if(base > 64) {//36
        _str = '\0';
        return NULL;
    }

    if(val < 0) {
        *_str++ = '-';
    }

    r = _uldiv(val, base);

    if(r.quot > 0) {
        _str = _ultostr(_str, r.quot, base);
    }

    *_str++ = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ,."[(int)r.rem];
    *_str   = '\0';
    return _str;
}

char *strsplit(const void *string_org, int org_len, const char *demial, char **last, int *len)
{
    unsigned char *str;
    unsigned char *p;

    if(org_len < 1 || !string_org || !demial) {
        return NULL;
    }

    if(*last) {
        if(*last == string_org) {
            *last = NULL;
            return NULL;
        }

        str = (unsigned char *)*last;

    } else {
        str = (unsigned char *)string_org;
    }

    if(!str) {
        return (char *)str;
    }

    p = str;

    while(p < (unsigned char *)string_org + org_len && *p != demial[0]) {
        p++;
    }

    if(p == (unsigned char *)string_org + org_len) {
        *last = (char *)string_org;

    } else {
        *last = (char *)p + 1;
    }

    if(str) {
        if(*last != string_org) {
            *len = (*last - (char *)str) - 1;

        } else {
            *len = (unsigned char *)(string_org + org_len) - str;
        }
    }

    return (char *)str;
}

