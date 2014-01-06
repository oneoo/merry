#include "smp.h"

/*
simple memory pool
*/

#define MAX_SMP_SIZE 2048*32 // total size = 2048*2048*32 = 128MB, max block = 64KB

/*
    ---------------------------
    | 32 | 64 | ... | 2048*32 |
    | 32 | 64 | ... | ------- | // count 1
    | .. | .. | ... |         |
    | 32 | -- | ... |         | // count (2048*32)/64 = 1024
    | 32 | .. | ... |         |
    | -- |    |     |         | // count 2048
    ---------------------------
*/

static void *head[MAX_SMP_SIZE / 32] = {0};
static unsigned short link_c[MAX_SMP_SIZE / 32] = {0};

#define _SSIZE (sizeof(unsigned short))
#define _ISIZE (sizeof(unsigned int))
#define _I_SSIZE (_ISIZE + _SSIZE)
#define _S_PTR(p) (*(unsigned short *)(p))
#define _I_PTR(p) (*(unsigned int *)(p))

void *smp_malloc(unsigned int size)
{
    if(size % 32 > 0) {
        size += 32 - (size % 32);
    }

    if(size < 1) {
        return NULL;
    }

    if(size > MAX_SMP_SIZE) {
        void *p = malloc(size + _I_SSIZE);

        if(!p) {
            return NULL;
        }

        _I_PTR(p) = size;
        _S_PTR(p + _ISIZE) = 0;

        return p + _I_SSIZE;
    }

    short k = (size / 32) % (MAX_SMP_SIZE / 32);
    void *p = NULL;

    if(head[k] == NULL) {
        p = malloc(_SSIZE + size);

        if(!p) {
            return NULL;
        }

        _S_PTR(p) = size / 32;

    } else {
        link_c[k] --;
        p = head[k];
        head[k] = *(void **)(p + _SSIZE);

    }

    return p + _SSIZE;
}

void *smp_realloc(void *p, unsigned int _size)
{
    if(!p) {
        return NULL;
    }

    if(_size % 32 > 0) {
        _size += 32 - (_size % 32);
    }

    unsigned int old_size = 0;

    if(_S_PTR(p - _SSIZE) == 0) {
        old_size = _I_PTR(p - _I_SSIZE);

    } else {
        old_size = _S_PTR(p - _SSIZE) * 32;
    }

    if(_size <= old_size) {
        return p;
    }

    void *t = smp_malloc(_size);

    if(!t) {
        return NULL;
    }

    if(_S_PTR(p - _SSIZE) == 0) {
        memcpy(t, p, _I_PTR(p - _I_SSIZE));

    } else {
        memcpy(t, p, _S_PTR(p - _SSIZE) * 32);
    }

    smp_free(p);

    return t;
}

int smp_free(void *p)
{
    if(!p) {
        return 0;
    }

    if(_S_PTR(p - _SSIZE) == 0) {
        free(p - _I_SSIZE);
        return 1;
    }

    p = p - _SSIZE;

    short k = (_S_PTR(p)) % (MAX_SMP_SIZE / 32);

    if(link_c[k]++ >= MAX_SMP_SIZE / (_S_PTR(p) * 32)) {
        free(p);
        return 1;
    }

    *(void **)(p + _SSIZE) = head[k];

    head[k] = p;

    return 1;
}

#ifdef SMP_DEBUG

void main()
{
    char *a = smp_malloc(10240);
    char *b = smp_malloc(32);
    smp_free(a);
    smp_free(b);
    a = smp_malloc(10240);
    b = smp_malloc(32);
    smp_free(b);
    smp_free(a);
    a = smp_malloc(10240);
    b = smp_malloc(32);

    a = smp_realloc(a, 10340);
    memset(a, 'a', 10340);
    a[10339] = '\0';
    printf("%s\n", a + 10300);

    b = smp_realloc(b, 64);
    memset(b, 'b', 64);
    b[63] = '\0';
    printf("%s\n", b);

    smp_free(a);
    smp_free(b);

    a = smp_malloc(2048 * 32);
    smp_free(a);
    a = smp_malloc(2048 * 32);
    b = smp_malloc(2048 * 32);
    smp_free(a);
    smp_free(b);
    exit(0);
}

#endif
