#include <errno.h>
#include <sys/socket.h>
#include <sys/uio.h>
#ifdef linux
#include <sys/sendfile.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "smp.h"

#define close(fd) do { if (fd >= 0) { close(fd); fd = -1; } } while (0)
#define NOAGAIN (errno != EAGAIN && errno != EWOULDBLOCK)

#ifndef _NETWORK_H
#define _NETWORK_H

#define EP_D_BUF_SIZE 4096

int set_nonblocking(int fd, int blocking);
int network_bind(const char *addr, int port);
int network_raw_send(int client_fd, const char *contents, int length);
char *network_raw_read(int cfd, int *datas_len);
int network_raw_sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

#endif
