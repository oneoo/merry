#include "se-util.h"

static se_be_accept_cb be_accept = NULL;
static int _server_fd = 0;

static struct hostent *localhost_ent = NULL;
static uint16_t dns_tid = 0;
static struct sockaddr_in dns_servers[4];
static int dns_server_count = 0;
static unsigned char buf_4096[4096];
static struct sockaddr_in rt_addr = {0};
static struct sockaddr_in null_addr = {0};
static void *dns_cache[3][64] = {{0}, {0}, {0}};
static int dns_cache_ttl = 180;

int se_errno = 0;

static int be_accept_f(se_ptr_t *ptr)
{
    int acc_trys = 0, client_fd = -1;
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    while(acc_trys++ < 3) {
#ifdef HAVE_ACCPEPT4
        client_fd = accept4(server_fd, (struct sockaddr *)&remote_addr, &addr_len, SOCK_NONBLOCK);

        if(errno == ENOSYS) {
            client_fd = accept(server_fd, (struct sockaddr *) &remote_addr, &addr_len);
        }

#else
        client_fd = accept(server_fd, (struct sockaddr *) &remote_addr, &addr_len);
#endif

        if(client_fd < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            break;

        }

        if(client_fd < 0) {
            continue;
        }

        be_accept(client_fd, remote_addr.sin_addr);

        acc_trys = 0;
    }

    return 1;
}

int se_accept(int loop_fd, int __server_fd, se_be_accept_cb _be_accept)
{
    _server_fd = __server_fd;
    be_accept = _be_accept;
    se_ptr_t *se = se_add(loop_fd, _server_fd, NULL);
    se_be_read(se, be_accept_f);
    return 1;
}

void add_dns_cache(const char *name, struct in_addr addr, int do_recache)
{
    int p = (now / dns_cache_ttl) % 3;

    if(do_recache == 1) {
        p = (p + 1) % 3;
    }

    dns_cache_item_t *n = NULL,
                      *m = NULL;
    int nlen = strlen(name);
    uint32_t key1 = fnv1a_32((unsigned char *) name, nlen);
    uint32_t key2 = fnv1a_64((unsigned char *) name, nlen);
    int k = key1 % 64;
    n = dns_cache[p][k];

    if(n == NULL) {
        m = smp_malloc(sizeof(dns_cache_item_t));

        if(m == NULL) {
            return;
        }

        m->key1 = key1;
        m->key2 = key2;
        m->next = NULL;
        m->recached = do_recache;
        memcpy(&m->addr, &addr, sizeof(struct in_addr));
        dns_cache[p][k] = m;

    } else {
        while(n != NULL) {
            if(n->key1 == key1 && n->key2 == key2) {
                return; /// exists
            }

            if(n->next == NULL) {    /// last
                m = smp_malloc(sizeof(dns_cache_item_t));

                if(m == NULL) {
                    return;
                }

                m->key1 = key1;
                m->key2 = key2;
                m->next = NULL;
                m->recached = do_recache;
                memcpy(&m->addr, &addr, sizeof(struct in_addr));
                n->next = m;
                return;
            }

            n = (dns_cache_item_t *) n->next;
        }
    }
}

int get_dns_cache(const char *name, struct in_addr *addr)
{
    int p = (now / dns_cache_ttl) % 3;
    dns_cache_item_t *n = NULL,
                      *m = NULL;
    /// clear old caches
    int q = (p + 2) % 3;
    int i = 0;

    for(i = 0; i < 64; i++) {
        n = dns_cache[q][i];

        while(n) {
            m = n;
            n = n->next;
            smp_free(m);
        }

        dns_cache[q][i] = NULL;
    }

    /// end
    int nlen = strlen(name);
    uint32_t key1 = fnv1a_32((unsigned char *) name, nlen);
    uint32_t key2 = fnv1a_64((unsigned char *) name, nlen);
    n = dns_cache[p][key1 % 64];

    while(n != NULL) {
        if(n->key1 == key1 && n->key2 == key2) {
            break;
        }

        n = (dns_cache_item_t *) n->next;
    }

    if(n) {
        memcpy(addr, &n->addr, sizeof(struct in_addr));

        if(n->recached != 1) {
            n->recached = 1;
            add_dns_cache(name, n->addr, 1);
        }

        return 1;
    }

    return 0;
}

int be_get_dns_result(se_ptr_t *ptr)
{
    _se_util_epdata_t *epd = ptr->data;

    int len = 0;

    while((len = recvfrom(epd->fd, buf_4096, 2048, 0, NULL, NULL)) > 0 && len >= sizeof(dns_query_header_t)) {
        delete_timeout(epd->timeout_ptr);
        int _dns_query_fd = epd->fd;
        close(epd->fd);
        epd->fd = -1;
        se_delete(epd->se_ptr);

        const unsigned char *p = NULL,
                             *e = NULL;
        dns_query_header_t *header = NULL;
        uint16_t type = 0;
        int found = 0, stop = 0, dlen = 0, nlen = 0;
        int err = 0;
        header = (dns_query_header_t *) buf_4096;

        if(ntohs(header->nqueries) != 1) {
            err = 1;
        }

        if(header->tid != epd->dns_tid) {
            err = 1;
        }

        /* Skip host name */
        if(err == 0) {
            for(e = buf_4096 + len, nlen = 0, p = &header->data[0]; p < e
                && *p != '\0'; p++) {
                nlen++;
            }
        }

        /* We sent query class 1, query type 1 */
        if(&p[5] > e || _NTOHS(p + 1) != 0x01) {
            err = 1;
        }

        struct in_addr ips[10];

        /* Go to the first answer section */
        if(err == 0) {
            p += 5;

            /* Loop through the answers, we want A type answer */
            for(found = stop = 0; !stop && &p[12] < e;) {
                /* Skip possible name in CNAME answer */
                if(*p != 0xc0) {
                    while(*p && &p[12] < e) {
                        p++;
                    }

                    p--;
                }

                type = htons(((uint16_t *) p) [1]);

                if(type == 5) {
                    /* CNAME answer. shift to the next section */
                    dlen = htons(((uint16_t *) p) [5]);
                    p += 12 + dlen;

                } else if(type == 0x01) {
                    dlen = htons(((uint16_t *) p) [5]);
                    p += 12;

                    if(p + dlen <= e) {
                        memcpy(&ips[found], p, dlen);
                    }

                    p += dlen;

                    if(++found == header->nanswers) {
                        stop = 1;
                    }

                    if(found >= 10) {
                        break;
                    }

                } else {
                    stop = 1;
                }
            }
        }

        if(found > 0) {
            rt_addr.sin_addr = ips[_dns_query_fd % found];

            add_dns_cache(epd->dns_query_name, rt_addr.sin_addr, 0);

            epd->cb(epd->data, rt_addr);
            smp_free(epd);

            return 0;
        }

        smp_free(epd);
        break;
    }

    return 0;
}

static void dns_query_timeout_handle(void *ptr)
{
    _se_util_epdata_t *epd = ptr;
    close(epd->fd);
    delete_timeout(epd->timeout_ptr);
    se_errno = SE_DNS_QUERY_TIMEOUT;
    epd->cb(epd->data, null_addr);
    se_errno = 0;
    smp_free(epd);
}

int se_dns_query(int loop_fd, const char *name, int timeout, se_be_dns_query_cb cb, void *data)
{
    if(get_dns_cache(name, &rt_addr.sin_addr)) {
        cb(data, rt_addr);
        return 1;
    }

    if(dns_server_count == 0) {    /// init dns servers
        int p1 = 0,
            p2 = 0,
            p3 = 0,
            p4 = 0,
            i = 0;

        for(i = 0; i < 4; i++) {
            dns_servers[i].sin_family = AF_INET;
            dns_servers[i].sin_port = htons(53);
        }

        FILE *fp = NULL;
        char line[200],
             *p = NULL;

        if((fp = fopen("/etc/resolv.conf" , "r")) != NULL) {
            while(fgets(line , 200 , fp)) {
                if(line[0] == '#') {
                    continue;
                }

                if(strncmp(line , "nameserver" , 10) == 0) {
                    p = strtok(line , " ");
                    p = strtok(NULL , " ");

                    //p now is the dns ip :)
                    if(sscanf(p, "%d.%d.%d.%d", &p1, &p2, &p3, &p4) == 4) {
                        dns_servers[dns_server_count].sin_addr.s_addr = htonl(((p1 << 24) |
                                (p2 << 16) | (p3 << 8) | (p4)));
                        dns_server_count ++;
                    }

                    if(dns_server_count > 1) {
                        break;
                    }
                }
            }

            fclose(fp);
        }

        if(dns_server_count < 2) {
            dns_servers[dns_server_count].sin_addr.s_addr = inet_addr("8.8.8.8");
            dns_server_count++;
        }

        if(dns_server_count < 2) {
            dns_servers[dns_server_count].sin_addr.s_addr = inet_addr("208.67.22.222");
            dns_server_count++;
        }
    }

    if(++dns_tid > 65535 - 1) {
        dns_tid = 1;
    }

    int fd = socket(PF_INET, SOCK_DGRAM, 17);

    if(fd < 0) {
        return 0;
    }

    _se_util_epdata_t *epd = smp_malloc(sizeof(_se_util_epdata_t));

    if(!epd) {
        close(fd);
        return 0;
    }

    int nlen = strlen(name);

    if(nlen < 60) {
        memcpy(epd->dns_query_name, name, nlen);
        epd->dns_query_name[nlen] = '\0';

    } else {
        epd->dns_query_name[0] = '-';
        epd->dns_query_name[1] = '\0';
        nlen = 1;
    }

    name = epd->dns_query_name;

    epd->fd = fd;
    epd->cb = cb;
    epd->data = data;
    epd->dns_tid = dns_tid;

    int opt = 1;
    ioctl(epd->fd, FIONBIO, &opt);

    int i = 0, n = 0, m = 0;
    dns_query_header_t *header = NULL;
    const char *s;
    char *p;
    header           = (dns_query_header_t *) buf_4096;
    header->tid      = dns_tid;
    header->flags    = htons(0x100);
    header->nqueries = htons(1);
    header->nanswers = 0;
    header->nauth    = 0;
    header->nother   = 0;
    // Encode DNS name
    p = (char *) &header->data;   /* For encoding host name into packet */

    do {
        if((s = strchr(name, '.')) == NULL) {
            s = name + nlen;
        }

        n = s - name;           /* Chunk length */
        *p++ = n;               /* Copy length */

        for(i = 0; i < n; i++) {    /* Copy chunk */
            *p++ = name[i];
        }

        if(*s == '.') {
            n++;
        }

        name += n;
        nlen -= n;
    } while(*s != '\0');

    *p++ = 0;           /* Mark end of host name */
    *p++ = 0;           /* Well, lets put this byte as well */
    *p++ = 1;           /* Query Type */
    *p++ = 0;
    *p++ = 1;           /* Class: inet, 0x0001 */
    n = (unsigned char *) p - buf_4096;      /* Total packet length */

    epd->se_ptr = se_add(loop_fd, epd->fd, epd);
    se_be_read(epd->se_ptr, be_get_dns_result);

    if((m = sendto(
                epd->fd,
                buf_4096,
                n,
                0,
                (struct sockaddr *) &dns_servers[epd->fd % dns_server_count],
                sizeof(struct sockaddr)
            )
       ) != n) {
        se_delete(epd->se_ptr);
        close(epd->fd);
        return 0;
    }

    sendto(
        epd->fd,
        buf_4096,
        n,
        0,
        (struct sockaddr *) &dns_servers[(epd->fd + 1) % dns_server_count],
        sizeof(struct sockaddr)
    );

    epd->timeout_ptr = add_timeout(epd, timeout, dns_query_timeout_handle);

    return 1;
}

static int se_set_nonblocking(int fd, int nonblocking)
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

static int __be_connect(se_ptr_t *ptr)
{
    _se_util_epdata_t *epd = ptr->data;
    se_delete(epd->se_ptr);
    int result = 0;
    socklen_t result_len = sizeof(result);

    if(getsockopt(epd->fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
        // not connected, try next event
        return 0;
    }

    if(result != 0) {
        close(epd->fd);
        epd->fd = -1;
    }

    delete_timeout(epd->timeout_ptr);
    epd->connect_cb(epd->data, epd->fd);
    smp_free(epd);

    return 1;
}

static void connect_timeout_handle(void *ptr)
{
    _se_util_epdata_t *epd = ptr;
    close(epd->fd);
    delete_timeout(epd->timeout_ptr);
    se_errno = SE_CONNECT_TIMEOUT;
    epd->connect_cb(epd->data, -1);
    se_errno = 0;
    smp_free(epd);
}

static void be_dns_query(void *data, struct sockaddr_in addr)
{
    _se_util_epdata_t *epd = (_se_util_epdata_t *)data;
    epd->se_ptr = se_add(epd->loop_fd, epd->fd, epd);
    se_be_write(epd->se_ptr, __be_connect);
    addr.sin_port = htons(epd->port);

    int ret = connect(epd->fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));

    if(ret == 0) {
        se_delete(epd->se_ptr);
        epd->connect_cb(epd->data, epd->fd);
        smp_free(epd);
        return;

    } else if(ret == -1 && errno != EINPROGRESS) {
        close(epd->fd);
        se_delete(epd->se_ptr);
        epd->connect_cb(epd->data, -1);
        smp_free(epd);
        return;
    }
}

int se_connect(int loop_fd, const char *host, int port, int timeout, se_be_connect_cb _be_connect, void *data)
{
#ifndef linux

    if(port < 1) {
        return 0;
    }

#endif

    int fd = -1;

    if((fd = socket(port > 0 ? AF_INET : AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return 0;
    }

    if(!se_set_nonblocking(fd , 1)) {
        close(fd);
        return 0;
    }

    _se_util_epdata_t *epd = smp_malloc(sizeof(_se_util_epdata_t));

    if(!epd) {
        close(fd);
        return 0;
    }

    epd->port = port;
    epd->connect_cb = _be_connect;
    epd->fd = fd;
    epd->data = data;
    epd->timeout_ptr = NULL;

#ifdef linux

    if(port < 1) { /// connect to unix domain socket
        epd->se_ptr = se_add(loop_fd, fd, epd);
        se_be_write(epd->se_ptr, __be_connect);

        struct sockaddr_un un;

        memset(&un, 0, sizeof(struct sockaddr_un));
        strcpy(un.sun_path, host);
        un.sun_family = AF_UNIX;
        int length = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);

        int ret = connect(fd, (struct sockaddr *) &un, length);

        if(ret == 0) {
            se_delete(epd->se_ptr);
            epd->connect_cb(data, fd);
            smp_free(epd);
            return 1;

        } else if(ret == -1 && errno != EINPROGRESS) {
            close(epd->fd);
            se_delete(epd->se_ptr);
            epd->connect_cb(data, -1);
            smp_free(epd);
            return 0;
        }

        epd->timeout_ptr = add_timeout(epd, timeout, connect_timeout_handle);
        return 1;
    }

#endif

    rt_addr.sin_family = AF_INET;
    rt_addr.sin_port = htons(port);
    rt_addr.sin_addr.s_addr = inet_addr(host); // 按IP初始化

    if(rt_addr.sin_addr.s_addr == INADDR_NONE) { // 如果输入的是域名
        if(strcmp(host, "localhost") == 0) {
            if(localhost_ent == NULL) {
                localhost_ent = malloc(sizeof(struct hostent));
                struct hostent *phost = (struct hostent *) gethostbyname(host);
                memcpy(localhost_ent, phost, sizeof(struct hostent));
            }

            rt_addr.sin_addr.s_addr = ((struct in_addr *) localhost_ent->h_addr_list[0])->s_addr;

        } else if(get_dns_cache(host, &rt_addr.sin_addr)) {
        } else {
            //epd->timeout_ptr = add_timeout(epd, timeout, connect_timeout_handle);
            epd->loop_fd = loop_fd;
            se_dns_query(loop_fd, host, timeout, be_dns_query, epd);
            return 1;
        }
    }

    epd->se_ptr = se_add(loop_fd, fd, epd);
    se_be_write(epd->se_ptr, __be_connect);

    int ret = connect(fd, (struct sockaddr *) &rt_addr, sizeof(struct sockaddr_in));

    if(ret == 0) {
        se_delete(epd->se_ptr);
        epd->connect_cb(data, fd);
        smp_free(epd);
        return 1;

    } else if(ret == -1 && errno != EINPROGRESS) {
        close(epd->fd);
        se_delete(epd->se_ptr);
        epd->connect_cb(data, -1);
        smp_free(epd);
        return 0;
    }

    epd->timeout_ptr = add_timeout(epd, timeout, connect_timeout_handle);
    return 1;
}
