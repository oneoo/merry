#include "se.h"

#ifdef linux
#include <sys/epoll.h>
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

static struct epoll_event events[SE_SIZE], ev;
static unsigned long working_io_count = 0;
static int in_loop = 0;
static unsigned long quitime = 0;

/* libeio */
static int eio_inited = 0;
static int eio_respipe [2];
void eio_want_poll(void)
{
    char dummy = 0;
    write(eio_respipe [1], &dummy, 1);
}

void eio_done_poll(void)
{
    char dummy = 0;
    read(eio_respipe [0], &dummy, 1);
}

void eio_event_loop(void)
{
    struct pollfd pfd;
    pfd.fd     = eio_respipe [0];
    pfd.events = POLLIN;

    while(eio_nreqs()) {
        poll(&pfd, 1, -1);
        eio_poll();
    }
}
/* end */

int se_create(int event_size)
{
    if(eio_inited == 0) {
        eio_inited = 1;

        if(pipe(eio_respipe) || eio_init(eio_want_poll, eio_done_poll)) {
            eio_inited = 0;
        }
    }

    return epoll_create(event_size);
}

int se_loop(int loop_fd, int waitout, se_waitout_proc_t waitout_proc)
{
    int n = 0, i = 0, r = 1;
    se_ptr_t *ptr = NULL;

    in_loop = 1;

    while(1) {
        update_time();
        check_timeouts();
        eio_event_loop();

        if(waitout_proc) {
            r = waitout_proc();
        }

        n = epoll_wait(loop_fd, events, SE_SIZE, waitout);

        for(i = 0; i < n; i++) {
            ptr = events[i].data.ptr;

            if(events[i].events & (EPOLLIN | EPOLLET) && ptr->rfunc) {
                ptr->rfunc(ptr);

            } else if(events[i].events & (EPOLLOUT | EPOLLET) && ptr->wfunc) {
                ptr->wfunc(ptr);
            }
        }

        if(!r || (n == -1 && errno != EINTR)) {

            break;

        } else if(check_process_for_exit()) {
            if(working_io_count > 0 && (quitime == 0 || time(NULL) - quitime < 10)) {
                if(quitime == 0) {
                    quitime = time(NULL);
                }

                continue;
            }

            quitime = 0;
            on_process_exit_handler(0, NULL, NULL);

            break;
        }

    }

    in_loop = 0;

    return 0;
}

se_ptr_t *se_add(int loop_fd, int fd, void *data)
{
    se_ptr_t *ptr = malloc(sizeof(se_ptr_t));

    if(!ptr) {
        return NULL;
    }

    ptr->loop_fd = loop_fd;
    ptr->fd = fd;
    ptr->rfunc = NULL;
    ptr->wfunc = NULL;
    ptr->data = data;

    ev.data.ptr = ptr;
    ev.events = EPOLLPRI;

    int ret = epoll_ctl(loop_fd, EPOLL_CTL_ADD, fd, &ev);

    if(ret < 0) {
        free(ptr);
        ptr = NULL;
    }

    working_io_count += in_loop;

    return ptr;
}

int se_delete(se_ptr_t *ptr)
{
    if(!ptr) {
        return -1;
    }

    if(epoll_ctl(ptr->loop_fd, EPOLL_CTL_DEL, ptr->fd, &ev) < 0) {
        return -1;
    }

    free(ptr);

    working_io_count -= in_loop;

    return 0;
}

int se_be_read(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = func;
    ptr->wfunc = NULL;

    ev.data.ptr = ptr;
    ev.events = EPOLLIN | EPOLLET;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

int se_be_write(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = NULL;
    ptr->wfunc = func;

    ev.data.ptr = ptr;
    ev.events = EPOLLOUT | EPOLLET;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

int se_be_pri(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = func;
    ptr->wfunc = NULL;

    ev.data.ptr = ptr;
    ev.events = EPOLLPRI;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

int se_be_rw(se_ptr_t *ptr, se_rw_proc_t rfunc, se_rw_proc_t wfunc)
{
    ptr->rfunc = rfunc;
    ptr->wfunc = wfunc;

    ev.data.ptr = ptr;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

#endif
