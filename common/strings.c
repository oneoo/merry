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

unsigned long _strtol(char *str62, int base)
{
    int i, j, nResult = 0;

    for(i = 0; i < strlen(str62); i++) {
        j = str62[i] == ',' ? 62 : (str62[i] == '.' ? 63 : (str62[i] <= '9' ? str62[i] - '0' :
                                    (str62[i] <= 'Z' ? 36 + str62[i] - 'A' : 10 + str62[i] - 'a')));
        nResult += pow(base, (strlen(str62) - i - 1)) * j ;
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
        str = ltostr(str, r.quot, base);
    }

    *str++ = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ,."[(int)r.rem];
    *str   = '\0';
    return str;
}
