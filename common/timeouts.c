#include "timeouts.h"
#include "smp.h"

#define TIMEOUTS_LINK_RING_SIZE 6000
static timeout_t *timeout_links[TIMEOUTS_LINK_RING_SIZE] = {0};
static timeout_t *timeout_link_ends[TIMEOUTS_LINK_RING_SIZE] = {0};
static unsigned long last_check_time = 0;

timeout_t *add_timeout(void *ptr, int timeout, timeout_handle_cb handle)
{
    if(timeout < 1 || !handle) {
        return NULL;
    }

    timeout_t *n = malloc(sizeof(timeout_t));

    if(!n) {
        return NULL;
    }

    n->handle = handle;
    n->ptr = ptr;
    n->timeout = (longtime() + timeout) / 10;
    n->uper = NULL;
    n->next = NULL;

    unsigned long k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(timeout_link_ends[k] == NULL) {
        timeout_links[k] = n;
        timeout_link_ends[k] = n;

    } else { // add to link end
        timeout_link_ends[k]->next = n;
        n->uper = timeout_link_ends[k];
        timeout_link_ends[k] = n;
    }

    return n;
}

int check_timeouts()
{
    unsigned long l_now = (longtime() / 10);
    timeout_t *m = NULL, *n = NULL;
    int b = 1;

    if(last_check_time == 0) {
        last_check_time = l_now;
    }

    do {
        unsigned long k = last_check_time % TIMEOUTS_LINK_RING_SIZE;

        while(1) {
            b = 1;
            m = timeout_links[k];
            n = NULL;

            while(m) {
                n = m;
                m = m->next;

                if(l_now >= n->timeout) {
                    n->handle(n->ptr);
                    b = 0;
                    break;
                }
            }

            if(b) {
                break;
            }
        }

        if(last_check_time < l_now) {
            last_check_time++;
            continue;
        }
    } while(last_check_time < l_now);

    return 1;
}

void delete_timeout(timeout_t *n)
{
    if(!n) {
        return;
    }

    unsigned long k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(n->uper) {
        ((timeout_t *) n->uper)->next = n->next;

    } else {
        timeout_links[k] = n->next;
    }

    if(n->next) {
        ((timeout_t *) n->next)->uper = n->uper;

    } else {
        timeout_link_ends[k] = n->uper;
    }

    free(n);
}

void update_timeout(timeout_t *n, int timeout)
{
    if(!n) {
        return;
    }

    unsigned long k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(n->uper) {
        ((timeout_t *) n->uper)->next = n->next;

    } else {
        timeout_links[k] = n->next;
    }

    if(n->next) {
        ((timeout_t *) n->next)->uper = n->uper;

    } else {
        timeout_link_ends[k] = n->uper;
    }

    if(timeout < 1) {
        free(n);
        return;
    }

    n->timeout = (longtime() + timeout) / 10;
    n->uper = NULL;
    n->next = NULL;

    k = n->timeout % TIMEOUTS_LINK_RING_SIZE;

    if(timeout_link_ends[k] == NULL) {
        timeout_links[k] = n;
        timeout_link_ends[k] = n;

    } else { // add to link end
        timeout_link_ends[k]->next = n;
        n->uper = timeout_link_ends[k];
        timeout_link_ends[k] = n;
    }
}
