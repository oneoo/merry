#include "timeouts.h"
#include "smp.h"

extern time_t now;

static timeout_t *timeout_links[64] = {0};
static timeout_t *timeout_link_ends[64] = {0};

timeout_t *add_timeout(void *ptr, int timeout, timeout_handle_cb handle)
{
    if(timeout < 1 || !handle) {
        return NULL;
    }

    timeout_t *n = smp_malloc(sizeof(timeout_t));

    if(!n) {
        return NULL;
    }

    n->handle = handle;
    n->ptr = ptr;
    n->timeout = now + timeout;
    n->uper = NULL;
    n->next = NULL;

    int k = n->timeout % 64;

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
    int k = now % 64;

    timeout_t *m = timeout_links[k], *n = NULL;

    while(m) {
        n = m;
        m = m->next;

        if(now >= n->timeout) {
            n->handle(n->ptr);
        }
    }

    return 1;
}

void delete_timeout(timeout_t *n)
{
    if(!n) {
        return;
    }

    int k = n->timeout % 64;

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

    smp_free(n);
}

void update_timeout(timeout_t *n, int timeout)
{
    if(!n) {
        return;
    }

    int k = n->timeout % 64;

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
        smp_free(n);
        return;
    }

    n->timeout = now + timeout;
    n->uper = NULL;
    n->next = NULL;

    k = n->timeout % 64;

    if(timeout_link_ends[k] == NULL) {
        timeout_links[k] = n;
        timeout_link_ends[k] = n;

    } else { // add to link end
        timeout_link_ends[k]->next = n;
        n->uper = timeout_link_ends[k];
        timeout_link_ends[k] = n;
    }
}
