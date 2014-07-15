#include "shm.h"
#include <stdio.h>
#include <errno.h>

extern char process_chdir[924];
extern int is_daemon;
static int shm_ftok_id = 1;

shm_t *shm_malloc(size_t size)
{
    int oflag, sem_id, shm_id;
    union semun arg;

    /* create and init a shared memory segment for the counter */
    oflag = 0600 | IPC_CREAT;

    if((shm_id = shmget(ftok(process_chdir, shm_ftok_id), size, oflag)) < 0) {
        perror("shmget error\n");
        return NULL;
    }

    void *p = NULL;

    if((p = shmat(shm_id, NULL, 0)) < 0) {
        return NULL;
    }

    if((sem_id = semget(ftok(process_chdir, shm_ftok_id), 1, oflag)) < 0) {
        if(shmctl(shm_id, IPC_RMID, NULL) == -1) {
            perror("shmctl del error\n");
        }

        perror("semget error\n");
        return NULL;
    }

    arg.val = 1; /* binary semaphore */

    if(semctl(sem_id, 0, SETVAL, arg) < 0) {
        return NULL;
    }

    shm_t *o = malloc(sizeof(shm_t));
    o->shm_id = shm_id;
    o->sem_id = sem_id;
    o->p = p;

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

    /* remove the semaphore and shm segment */
    if(semctl(shm->sem_id, 1, IPC_RMID, 0) == -1) {
        perror("semctl del error\n");
    }

    free(shm);
    shm = NULL;
}

/* P - decrement semaphore and wait */
int shm_lock(shm_t *shm)
{
    if(is_daemon == 0) {
        return 1;
    }

    static struct sembuf sembuf;

    sembuf.sem_num = 0;
    sembuf.sem_op = -1;
    sembuf.sem_flg = 0;

    int ret = 0;

    while(1) {
        ret = semop(shm->sem_id, &sembuf, 1);

        if(ret < 0) {
            if(errno == EINTR) {
                continue;

            } else {
                return -1;
            }
        }

        break;
    }

    return ret;
}

/* V - increment semaphore and signal */
int shm_unlock(shm_t *shm)
{
    if(is_daemon == 0) {
        return 1;
    }

    static struct sembuf sembuf;

    sembuf.sem_num = 0;
    sembuf.sem_op =  1;
    sembuf.sem_flg = 0;

    int ret = 0;

    while(1) {
        ret = semop(shm->sem_id, &sembuf, 1);

        if(ret < 0) {
            if(errno == EINTR) {
                continue;

            } else {
                return -1;
            }
        }

        break;
    }

    return ret;
}
