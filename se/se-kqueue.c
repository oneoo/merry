#include "se.h"

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(BSD)
#include <sys/event.h>
struct kevent events[SE_SIZE] = {{0}}, ev[3] = {{0}};
static unsigned long working_io_count = 0;
static int in_loop = 0;

int se_create(int event_size)
{
    return kqueue();
}

int se_loop(int loop_fd, int waitout, se_waitout_proc_t waitout_proc)
{
    int n = 0, i = 0, r = 1;
    se_ptr_t *ptr = NULL;

    in_loop = 1;

    waitout *= 1000000;

    struct timespec tmout = { waitout / 1000000000, waitout % 1000000000 };

    while(1) {
        update_time();
        check_timeouts();

        if(waitout_proc) {
            r = waitout_proc();
        }

        n = kevent(loop_fd, NULL, 0, events, SE_SIZE, &tmout);

        for(i = 0; i < n; i++) {
            ptr = events[i].udata;

            if(events[i].filter == EVFILT_READ && ptr->rfunc) {
                ptr->rfunc(ptr);

            } else if(events[i].filter == EVFILT_WRITE && ptr->wfunc) {
                ptr->wfunc(ptr);
            }
        }

        if(!r || (n == -1 && errno != EINTR) || (check_process_for_exit() && working_io_count == 0)) {
            if(check_process_for_exit()) {
                on_process_exit_handler(0, NULL, NULL);
            }

            break;
        }

    }

    in_loop = 0;

    return 0;
}

static int ev_set_fd(int loop_fd, int fd, unsigned ACT, se_ptr_t *ptr)
{
    EV_SET(&ev[0], fd, EVFILT_READ, ACT, 0, 0, ptr);
    EV_SET(&ev[1], fd, EVFILT_WRITE, ACT, 0, 0, ptr);
    EV_SET(&ev[2], fd, EVFILT_SIGNAL, ACT, 0, 0, ptr);
    return kevent(loop_fd, ev, 3, NULL, 0, NULL);
}

se_ptr_t *se_add(int loop_fd, int fd, void *data)
{
    se_ptr_t *ptr = malloc(sizeof(se_ptr_t));

    if(!ptr) {
        return ptr;
    }

    ptr->loop_fd = loop_fd;
    ptr->fd = fd;
    ptr->rfunc = NULL;
    ptr->wfunc = NULL;
    ptr->data = data;

    int ret = 0;

    ret = ev_set_fd(loop_fd, fd, EV_ADD, ptr);

    if(ret < 0) {
        free(ptr);
        ptr = NULL;
    }

    ret = ev_set_fd(loop_fd, fd, EV_DISABLE, ptr);

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

    ev_set_fd(ptr->loop_fd, ptr->fd, EV_DELETE, ptr);

    free(ptr);

    working_io_count -= in_loop;

    return 0;
}

int se_be_read(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = func;
    ptr->wfunc = NULL;

    ev_set_fd(ptr->loop_fd, ptr->fd, EV_DISABLE, ptr);

    EV_SET(&ev[0], ptr->fd, EVFILT_READ, EV_ENABLE, 0, 0, ptr);
    return kevent(ptr->loop_fd, ev, 1, NULL, 0, NULL);
}

int se_be_write(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = NULL;
    ptr->wfunc = func;

    ev_set_fd(ptr->loop_fd, ptr->fd, EV_DISABLE, ptr);

    EV_SET(&ev[0], ptr->fd, EVFILT_WRITE, EV_ENABLE, 0, 0, ptr);
    return kevent(ptr->loop_fd, ev, 1, NULL, 0, NULL);
}

int se_be_pri(se_ptr_t *ptr, se_rw_proc_t func)
{
    ptr->rfunc = func;
    ptr->wfunc = NULL;

    ev_set_fd(ptr->loop_fd, ptr->fd, EV_DISABLE, ptr);

    EV_SET(&ev[0], ptr->fd, EVFILT_SIGNAL, EV_ENABLE, 0, 0, ptr);
    return kevent(ptr->loop_fd, ev, 1, NULL, 0, NULL);
}

int se_be_rw(se_ptr_t *ptr, se_rw_proc_t rfunc, se_rw_proc_t wfunc)
{
    ptr->rfunc = rfunc;
    ptr->wfunc = wfunc;

    ev_set_fd(ptr->loop_fd, ptr->fd, EV_DISABLE, ptr);

    EV_SET(&ev[0], ptr->fd, EVFILT_READ, EV_ENABLE, 0, 0, ptr);
    EV_SET(&ev[1], ptr->fd, EVFILT_WRITE, EV_ENABLE, 0, 0, ptr);
    return kevent(ptr->loop_fd, ev, 2, NULL, 0, NULL);
}

#endif
