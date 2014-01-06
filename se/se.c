#include "se.h"

#ifdef linux
#include <sys/epoll.h>
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

static struct epoll_event events[SE_SIZE], ev;

int se_create(int event_size)
{
    return epoll_create(event_size);
}

int se_loop(int loop_fd, int waitout, se_waitout_proc_t waitout_proc)
{
    int n = 0, i = 0, r = 1;
    se_ptr_t *ptr = NULL;

    while(1) {
        update_time();
        check_timeouts();

        if(waitout_proc) {
            r = waitout_proc();
        }

        n = epoll_wait(loop_fd, events, SE_SIZE, waitout);

        for(i = 0; i < n; i++) {
            ptr = events[i].data.ptr;

            if(events[i].events & EPOLLIN && ptr->rfunc) {
                ptr->rfunc(ptr);

            } else if(events[i].events & EPOLLOUT && ptr->wfunc) {
                ptr->wfunc(ptr);
            }
        }

        if(!r || n == -1 || check_process_for_exit()) {
            break;
        }

    }

    return 0;
}

se_ptr_t *se_add(int loop_fd, int fd, void *data)
{
    se_ptr_t *ptr = smp_malloc(sizeof(se_ptr_t));

    if(!ptr) {
        return ptr;
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
        smp_free(ptr);
        ptr = NULL;
    }

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

    smp_free(ptr);

    return 0;
}

int se_be_read(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = func;

    ev.data.ptr = ptr;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

int se_be_write(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->wfunc = func;

    ev.data.ptr = ptr;
    ev.events = EPOLLOUT | EPOLLRDHUP | EPOLLHUP;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

int se_be_pri(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = func;

    ev.data.ptr = ptr;
    ev.events = EPOLLPRI;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

int se_be_rw(se_ptr_t *ptr, se_rw_proc_t rfunc, se_rw_proc_t wfunc)
{
    ptr->rfunc = rfunc;
    ptr->wfunc = wfunc;

    ev.data.ptr = ptr;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP;

    return epoll_ctl(ptr->loop_fd, EPOLL_CTL_MOD, ptr->fd, &ev);
}

#endif
