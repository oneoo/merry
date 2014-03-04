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

typedef struct {
    void *priv;
    void *next;
    char f[64];
    unsigned int l;
    char f2[64];
    unsigned int l2;
    void *p;
} smp_link_t;

static void *smp_link[32] = {0};
static char old_f[64] = {0};
static int old_l = 0;

static void add_to_smp_link(void *p, int s, char *f, int l)
{
    if(!p) {
        return ;
    }

    if(s % 32 > 0) {
        s += 32 - (s % 32);
    }

    s = s / 32;

    smp_link_t *n = malloc(sizeof(smp_link_t));
    n->priv = NULL;
    n->next = NULL;
    n->l = l;
    n->p = p;
    memset(n->f, 0, 64);
    memset(n->f2, 0, 64);

    int fl = strlen(f);
    int fs = 0;

    if(fl > 63) {
        fs = fl - 63;
        fl = 63;
    }

    memcpy(n->f, f + fs, fl);
    n->f[fl] = '\0';

    if(!old_l) {
        memcpy(n->f2, f + fs, fl);
        n->f2[fl] = '\0';
        n->l2 = l;

    } else {
        memcpy(n->f2, old_f, strlen(old_f));
        n->f2[fl] = '\0';
        n->l2 = old_l;
    }

    if(!smp_link[s % 32]) {
        smp_link[s % 32] = n;

    } else {
        n->next = smp_link[s % 32];
        ((smp_link_t *)smp_link[s % 32])->priv = n;
        smp_link[s % 32] = n;
    }
}

#ifdef SMPDEBUG
static void delete_in_smp_link(void *p, int s)
{
    if(!p) {
        return ;
    }

    if(s % 32 > 0) {
        s += 32 - (s % 32);
    }

    s = s / 32;

    smp_link_t *n = smp_link[s % 32];

    while(n) {
        if(n->p == p) {
            if(n->priv) {
                ((smp_link_t *)n->priv)->next = n->next;

                if(n->next) {
                    ((smp_link_t *)n->next)->priv = n->priv;
                }

            } else {
                if(n->next) {
                    ((smp_link_t *)n->next)->priv = n->priv;
                    smp_link[s % 32] = n->next;

                } else {
                    smp_link[s % 32] = NULL;
                }
            }

            void *m = n;
            n = n->next;
            free(m);
            return ;

        } else {
            n = n->next;
        }

    }

    printf("free error %p\n", p);
    exit(1);
}
#endif

void dump_smp_link()
{
#ifdef SMPDEBUG
    int i = 0;
    smp_link_t *n = NULL;

    for(i = 0; i < 32; i++) {
        printf("%d ========================================\n", i * 32);
        n = smp_link[i];

        while(n) {
            printf("%s:%d %s:%d %p\n", n->f2, n->l2, n->f, n->l, n->p);
            n = n->next;
        }
    }

#endif
}

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

void *_smp_malloc(unsigned int size, char *f, int l)
{
    void *r = smp_malloc(size);
    old_l = 0;
    add_to_smp_link(r, size > MAX_SMP_SIZE ? 0 : size, f, l);
    return r;
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

void *_smp_realloc(void *p, unsigned int size, char *f, int l)
{
    old_l = 0;
    int old_size = 0;

    if(_S_PTR(p - _SSIZE) == 0) {
        old_size = _I_PTR(p - _I_SSIZE);

    } else {
        old_size = _S_PTR(p - _SSIZE) * 32;
    }

    old_size = old_size / 32;

    smp_link_t *n = smp_link[old_size % 32];

    while(n) {
        if(n->p == p) {
            memcpy(old_f, n->f2, strlen(n->f2));
            old_f[strlen(n->f2)] = '\0';
            old_l = n->l2;
            break;

        } else {
            n = n->next;
        }

    }

    void *r = smp_realloc(p, size);

    if(r != p) {
        add_to_smp_link(r, size > MAX_SMP_SIZE ? 0 : size, f, l);
    }

    return r;
}

int smp_free(void *p)
{
    if(!p) {
        return 0;
    }

    if(_S_PTR(p - _SSIZE) == 0) {
#ifdef SMPDEBUG
        delete_in_smp_link(p, 0);
#endif
        free(p - _I_SSIZE);
        return 1;
    }

#ifdef SMPDEBUG
    void *o = p;
#endif
    p = p - _SSIZE;

    short k = (_S_PTR(p)) % (MAX_SMP_SIZE / 32);
#ifdef SMPDEBUG
    delete_in_smp_link(o, (_S_PTR(p)) * 32);
#endif

    if(link_c[k]++ >= MAX_SMP_SIZE / (_S_PTR(p) * 32)) {
        free(p);
        return 1;
    }

    *(void **)(p + _SSIZE) = head[k];

    head[k] = p;

    return 1;
}

int _smp_free(void *p, char *f, int l)
{
    return smp_free(p);
}

static int all_freed = 0;
void smp_free_all()
{
#ifdef SMPDEBUG

    if(all_freed++) {
        return;
    }

    int i = 0;
    smp_link_t *n = NULL, *m = NULL;
    void *p = NULL;

    for(i = 0; i < 32; i++) {
        n = smp_link[i];

        while(n) {
            m = n;
            n = n->next;
            p = m->p;

            if(_S_PTR(p - _SSIZE) == 0) {
                free((p - _I_SSIZE));

            } else {
                free((p - _SSIZE));
            }

            free(m);
        }
    }

    void *np = NULL;

    for(i = 0; i < MAX_SMP_SIZE / 32; i++) {
        np = head[i];

        while(np) {
            p = np;
            np = *(void **)(np + _SSIZE);
            free(p);
        }
    }

#endif
}

#ifdef SMP_DEBUG

#define malloc(s) _smp_malloc(s, __FILE__, __LINE__)
#define realloc(p,s) _smp_realloc(p,s, __FILE__, __LINE__)
#define free(p) _smp_free(p, __FILE__, __LINE__)

void main()
{
    char *a = malloc(10240);
    char *b = malloc(32);
    free(a);
    free(b);
    a = malloc(10240);
    b = malloc(32);
    free(b);
    free(a);
    a = malloc(10240);
    b = malloc(32);

    a = realloc(a, 10340);
    memset(a, 'a', 10340);
    a[10339] = '\0';
    printf("%s\n", a + 10300);

    b = realloc(b, 64);
    memset(b, 'b', 64);
    b[63] = '\0';
    printf("%s\n", b);
    free(a);
    free(b);

    a = malloc(2048 * 32 + 1);
    free(a);
    a = malloc(2048 * 32 + 1);
    b = malloc(MAX_SMP_SIZE - 1024);
    b = realloc(b, MAX_SMP_SIZE + 4096);
    dump_smp_link();
    free(a);
    free(b);

    exit(0);
}

#endif
