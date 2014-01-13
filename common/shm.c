#include "shm.h"

extern char process_chdir[924];
extern int is_daemon;
static int shm_ftok_id = 1;

shm_t *shm_malloc(size_t size)
{
    int oflag, sem_id, shm_id;
    union semun arg;

    /* create and init a shared memory segment for the counter */
    oflag = 0666 | IPC_CREAT;

    if((shm_id = shmget(ftok(process_chdir, shm_ftok_id), size, oflag)) < 0) {
        return NULL;
    }

    void *p = NULL;

    if((p = shmat(shm_id, NULL, 0)) < 0) {
        return NULL;
    }

    /* create and init the semaphore that will protect the counter */
    oflag = 0666 | IPC_CREAT;

    if((sem_id = semget(ftok(process_chdir, shm_ftok_id), 1, oflag)) < 0) {
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

    union semun arg;

    arg.val = 1; /* binary semaphore */

    /* remove the semaphore and shm segment */
    if(semctl(shm->sem_id, 0, IPC_RMID, arg) < 0) {
        return;
    }

    if(shmctl(shm->shm_id, IPC_RMID, NULL) < 0) {
        return;
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
    return semop(shm->sem_id, &sembuf, 1);
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
    return semop(shm->sem_id, &sembuf, 1);
}
