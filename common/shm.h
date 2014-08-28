#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <semaphore.h>
#include "smp.h"

#ifndef __SHM_H
#define __SHM_H

typedef struct _shm_t {
    int shm_id;
    int sem_id;
    void *p;
} shm_t;

#ifndef __APPLE__
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux specific) */
};
#endif

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
