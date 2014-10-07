#include <stdlib.h>
#include <sys/shm.h>
#include "smp.h"

#ifndef __SHM_H
#define __SHM_H

typedef struct _shm_t {
    int shm_id;
    void *p;
} shm_t;

shm_t *shm_malloc(size_t size);
void shm_free(shm_t *shm);
int shm_lock(shm_t *shm);
int shm_unlock(shm_t *shm);

#include <sched.h>

#define gcc_lock(lkp) do{ \
        while(!__sync_bool_compare_and_swap(lkp, 0, 1)){ \
            sched_yield(); \
        } \
    }while(0)

#define gcc_try_lock(lkp) ({ \
        (__sync_bool_compare_and_swap(lkp, 0, 1) ? 0 : -1); \
    })

#define gcc_unlock(lkp) do{ \
        *(lkp) = 0; \
    }while(0)

#endif
