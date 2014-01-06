#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "../common/times.h"
#include "../common/timeouts.h"
#include "../common/process.h"
#include "../common/smp.h"

#ifndef _SE_H
#define _SE_H

#define SE_SIZE 4096
typedef struct se_ptr_s se_ptr_t;
typedef int (*se_rw_proc_t)(se_ptr_t *ptr);
typedef int (*se_waitout_proc_t)();

struct se_ptr_s {
    unsigned int loop_fd;
    unsigned int fd;
    se_rw_proc_t rfunc;
    se_rw_proc_t wfunc;
    void *data;
};

int se_create(int event_size);
int se_loop(int loop_fd, int waitout, se_waitout_proc_t waitout_proc);
se_ptr_t *se_add(int loop_fd, int fd, void *data);
int se_delete(se_ptr_t *ptr);
int se_be_read(se_ptr_t *ptr, se_rw_proc_t func);
int se_be_write(se_ptr_t *ptr, se_rw_proc_t func);
int se_be_pri(se_ptr_t *ptr, se_rw_proc_t func);
int se_be_rw(se_ptr_t *ptr, se_rw_proc_t rfunc, se_rw_proc_t wfunc);

#include "se-util.h"

#endif // _SE_H
