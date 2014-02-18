#include "strings.h"

int stricmp(const char *str1, const char *str2)
{
    char *p1, *p2;
    int  i = 0, len = 0;

    if(str1 == NULL) {
        if(str2 != NULL) {
            return -1;
        }

        if(str2 == NULL) {
            return 0;
        }
    }

    p1 = (char *) str1;
    p2 = (char *) str2;
    len = (strlen(str1) < strlen(str2)) ? strlen(str1) : strlen(str2);

    for(i = 0; i < len; i++) {
        if(toupper(*p1) == toupper(*p2)) {
            p1++;
            p2++;

        } else {
            return toupper(*p1) - toupper(*p2);
        }
    }

    return strlen(str1) - strlen(str2);
}

char *stristr(const char *str, const char *pat, int length)
{
    if(!str || !pat) {
        return (NULL);
    }

    if(length < 1) {
        length = strlen(str);
    }

    int pat_len = strlen(pat);

    if(length < pat_len) {
        return NULL;
    }

    int i = 0;

    for(i = 0; i < length; i++) {
        if(toupper(str[i]) == toupper(pat[0]) && (length - i) >= pat_len
           && toupper(str[i + pat_len - 1]) == toupper(pat[pat_len - 1])) {
            int j = i;

            for(; j < i + pat_len; j++) {
                if(toupper(str[j]) != toupper(pat[j - i])) {
                    break;
                }
            }

            if(j < i + pat_len) {
                continue;
            }

            return (char *) str + i;
        }
    }

    return (NULL);
}

void random_string(char *string, size_t length, int s)
{
    /* Seed number for rand() */
    struct timeb t;
    ftime(&t);
    srand((unsigned int) 1000 * t.time + t.millitm + s);

    unsigned int num_chars = length;
    unsigned int i;
    unsigned int j = rand();

    for(i = 0; i < num_chars; ++i) {
        if(j % 1000 < 500) {
            string[i] = j % ('g' - 'a') + 'a';

        } else {
            string[i] = j % (':' - '0') + '0';
        }

        j = rand();
    }
}

unsigned long _strtol(char *str64, int base)
{
    int i, j, nResult = 0;
    char _t[32] = {0};

    for(i = 0; i < strlen(str64); i++) {
        if(str64[i] == '\r' || str64[i] == '\n' || str64[i] == '\t' || str64[i] == ' ' || str64[i] == ';') {
            break;
        }

        _t[i] = str64[i];
    }

    for(i = 0; i < strlen(_t); i++) {


        j = _t[i] == ',' ? 62 : (_t[i] == '.' ? 63 : (_t[i] <= '9' ? _t[i] - '0' :
                                 (_t[i] <= 'Z' ? 36 + _t[i] - 'A' : 10 + _t[i] - 'a')));
        nResult += pow(base, (strlen(_t) - i - 1)) * j ;
    }

    return nResult;
}

char *_ltostr(char *str, long val, unsigned base)
{
    ldiv_t r;

    if(base > 64) {//36
        str = '\0';
        return str;
    }

    if(val < 0) {
        *str++ = '-';
    }

    r = ldiv(labs(val), base);

    if(r.quot > 0) {
        str = _ltostr(str, r.quot, base);
    }

    *str++ = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ,."[(int)r.rem];
    *str   = '\0';
    return str;
}

char *strsplit(void *string_org, int org_len, const char *demial, char **last, int *len)
{
    unsigned char *str;
    unsigned char *p;

    if(*last) {
        if(*last == string_org) {
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

    if(*p && p < (unsigned char *)string_org + org_len) {
        *last = (char *)p + 1;

    } else {
        *last = string_org;
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

