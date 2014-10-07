#include "shm.h"
#include <stdio.h>
#include <errno.h>

extern char process_chdir[924];
extern int is_daemon;
static int shm_ftok_id = 1;

shm_t *shm_malloc(size_t size)
{
    int oflag, shm_id;

    /* create and init a shared memory segment for the counter */
    oflag = IPC_CREAT | SHM_R | SHM_W | IPC_EXCL;

    //ftok(process_chdir, shm_ftok_id)
    if((shm_id = shmget(IPC_PRIVATE, size + sizeof(int), oflag)) < 0) {
        perror("shmget error\n");
        return NULL;
    }

    void *p = NULL;

    if((p = shmat(shm_id, NULL, 0)) < 0) {
        return NULL;
    }

    memset(p, 0, size + sizeof(int));

    shm_t *o = malloc(sizeof(shm_t));
    o->shm_id = shm_id;
    o->p = p + sizeof(int);

    shm_ftok_id++;

    return o;
}

void shm_free(shm_t *shm)
{
    if(shm == NULL) {
        return;
    }

    if(shmctl(shm->shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl del error\n");
    }

    free(shm);
    shm = NULL;
}

int shm_lock(shm_t *shm)
{
    if(is_daemon == 0) {
        return 1;
    }

    gcc_lock((int *)((char *)shm->p - sizeof(int)));
    return 1;
}

int shm_unlock(shm_t *shm)
{
    if(is_daemon == 0) {
        return 1;
    }

    gcc_unlock((int *)((char *)shm->p - sizeof(int)));
    return 1;
}
