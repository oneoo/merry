#include "se.h"
#include <netdb.h>
#ifdef linux
#include <sys/un.h>
#include <stddef.h>
#endif

#include "../common/smp.h"
#include "../common/timeouts.h"

#ifndef _SE_UTIL_H
#define _SE_UTIL_H

#define _NTOHS(p) (((p)[0] << 8) | (p)[1])

typedef void (*se_be_accept_cb)(int fd, struct in_addr client_addr);
typedef void (*se_be_dns_query_cb)(void *data, struct sockaddr_in addr);
typedef void (*se_be_connect_cb)(void *data, int fd);
int se_accept(int loop_fd, int server_fd, se_be_accept_cb _be_accept);
int se_connect(int loop_fd, const char *host, int port, int timeout, se_be_connect_cb _be_connect, void *data);
int se_dns_query(int loop_fd, const char *name, se_be_dns_query_cb cb, void *data);

typedef struct {
    uint32_t    key1;
    uint32_t    key2;
    struct in_addr addr;
    int     recached;
    void       *next;
} dns_cache_item_t;

typedef struct {
    void *se_ptr;
    void *timeout_ptr;

    char dns_query_name[60];
    int fd;
    int dns_tid;
    int port;
    void *data;
    se_be_dns_query_cb cb;
    se_be_connect_cb connect_cb;
} _se_util_epdata_t;

typedef struct {
    uint16_t    tid;            /* Transaction ID */
    uint16_t    flags;          /* Flags */
    uint16_t    nqueries;       /* Questions */
    uint16_t    nanswers;       /* Answers */
    uint16_t    nauth;          /* Authority PRs */
    uint16_t    nother;         /* Other PRs */
    unsigned char   data[1];    /* Data, variable length */
} dns_query_header_t;

#endif
