#include "network.h"

extern time_t now;
extern struct tm _now_gtm;
extern struct tm _now_lc;
extern char now_gmt[32];
extern char now_lc[32];

int set_nonblocking(int fd, int nonblocking)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if(flags == -1) {
        return 0;
    }

    if(nonblocking) {
        flags |= O_NONBLOCK;

    } else {
        flags &= ~O_NONBLOCK;
    }

    return fcntl(fd, F_SETFL, flags) != -1;
}

int network_bind(const char *addr, int port)
{
    int fd = -1;
    struct sockaddr_in sin;

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("Socket failed");
        signal(SIGHUP, SIG_IGN);
        exit(1);
    }

    int reuseaddr = 1, nodelay = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuseaddr,
               sizeof(int));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &nodelay, sizeof(int));

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)(port));

    if(strlen(addr) > 6) {
        inet_aton(addr, & (sin.sin_addr));

    } else {
        sin.sin_addr.s_addr = INADDR_ANY;
    }

    if(bind(fd, (struct sockaddr *) &sin, sizeof(sin)) != 0) {
        sleep(1);

        if(bind(fd, (struct sockaddr *) &sin, sizeof(sin)) != 0) {
            perror("bind failed\n");
            signal(SIGHUP, SIG_IGN);
            exit(1);
        }
    }

    if(!set_nonblocking(fd , 1)) {
        perror("set nonblocking failed\n");
        exit(1);
    }

    if(listen(fd, 1024) != 0) {
        perror("listen failed\n");
        signal(SIGHUP, SIG_IGN);
        exit(1);
    }

    return fd;
}

int network_raw_send(int client_fd, const char *contents, int length)
{
    int len = 0, n;
    int a = 0;
    int max = length;

    while(1) {
        if(len >= length || length < 1) {
            break;
        }

        n = send(client_fd, contents + len, length - len, MSG_DONTWAIT);

        if(n < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                if(a++ > max) {
                    return 0;
                }

                continue;

            } else {
                return -1;
                break;
            }
        }

        len += n;
    }

    return len;
}

char *network_raw_read(int cfd, int *datas_len)
{
    char *datas = NULL;
    int len = 0;
    int n = 0;

    while(1) {
        if(datas == NULL) {
            datas = (char *) smp_malloc(sizeof(char) * EP_D_BUF_SIZE);
            memset(datas, 0, EP_D_BUF_SIZE);

        } else {
            datas = (char *) smp_realloc(datas, sizeof(char) * (len + EP_D_BUF_SIZE));
            memset(datas + len, 0, EP_D_BUF_SIZE);
        }

        if((n = read(cfd, datas + len, EP_D_BUF_SIZE)) <= 0) {
            if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }

            break;
        }

        len += n;

        if(datas[len - 3] == '\n' && datas[len - 1] == '\n') {
            break;
        }
    }

    if(datas[len - 3] == '\n' && datas[len - 1] == '\n') {
        datas[len - 4] = '\0';

    } else {
        smp_free(datas);
        datas = NULL;
        len = 0;
    }

    if(*datas_len) {
        *datas_len = len;
    }

    return datas;
}

int network_raw_sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
#if defined(__APPLE__) || defined(__FreeBSD__)
    off_t my_count = count;
    int rc;

    // We have to do this loop nastiness, because mac os x fails with resource
    // temporarily unavailable (per bug e8eddb51a8)
    do {
#if defined(__APPLE__)
        rc = sendfile(in_fd, out_fd, *offset, &my_count, NULL, 0);
#elif defined(__FreeBSD__)
        rc = sendfile(in_fd, out_fd, *offset, count, NULL, &my_count, 0);
#endif
        *offset += my_count;
    } while(rc != 0 && errno == 35);

    return my_count;
#else
    return sendfile(out_fd, in_fd, offset, count);
#endif
}
