#if defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 12)
#define _GNU_SOURCE
#include <sys/socket.h>
#define HAVE_ACCPEPT4 1
#endif
#endif

#include <netdb.h>
#include <sys/ioctl.h>
#ifdef linux
#include <sys/un.h>
#include <stddef.h>
#endif

#include "../merry.h"

#ifndef _SE_UTIL_H
#define _SE_UTIL_H

#define SE_CONNECT_TIMEOUT -1002
#define SE_DNS_QUERY_TIMEOUT -1001

typedef void (*se_be_accept_cb)(int fd, struct in_addr client_addr);
typedef void (*se_be_dns_query_cb)(void *data, struct sockaddr_in addr);
typedef void (*se_be_connect_cb)(void *data, int fd);
int se_set_nonblocking(int fd, int nonblocking);
int se_accept(int loop_fd, int server_fd, se_be_accept_cb _be_accept);
int se_dns_query(int loop_fd, const char *name, int timeout, se_be_dns_query_cb cb, void *data);
int se_connect(int loop_fd, const char *host, int port, int timeout, se_be_connect_cb _be_connect, void *data);

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
    int loop_fd;
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
