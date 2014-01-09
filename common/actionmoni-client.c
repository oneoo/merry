#include "actionmoni-client.h"

/*
    printf("%d\n", actionmoni_open("192.168.0.69", 1234));
    actionmoni_set_keys("aaa,bbb", 7);
    actionmoni_count("aaa"); //aaa+1
    actionmoni_counts("aaa", 10); //aaa+ 10
    actionmoni_ts("bbb", 10); // 10ms
    actionmoni_multi(3, // n of actions
        AC_NODE_ADD, "aaa",
        AC_NODE_ADD_CS, "aaa", 10,
        100+10, "bbb"
    );
    exit(0);
*/
static int actionmoni_fd = -1;
static struct sockaddr_in servaddr = {0};
static char buf_4096[4096] = {0};

static uint32_t fnv1_32(const char *data, uint32_t len)
{
    uint32_t rv = 2166136261;
    uint32_t i;

    for(i = 0; i < len; i++) {
        rv += (rv << 1) + (rv << 4) + (rv << 7) + (rv << 8) + (rv << 24);
        rv = rv ^ data[i];
    }

    return rv;
}

int actionmoni_open(const char *host, int port)
{
    actionmoni_fd = socket(AF_INET, SOCK_DGRAM, 0);

    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(host);
    servaddr.sin_port = htons(port);

    buf_4096[0] = '#';

    return actionmoni_fd;
}

int actionmoni_count(const char *_key)
{
    int len = 1;
    uint32_t ac = AC_NODE_ADD;
    memcpy(buf_4096 + len, &ac, sizeof(uint32_t));
    len += sizeof(uint32_t);

    uint32_t key = fnv1_32(_key, strlen(_key));
    memcpy(buf_4096 + len, &key, sizeof(uint32_t));
    len += sizeof(uint32_t);

    return actionmoni_fd > -1
           && sendto(actionmoni_fd, buf_4096, len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
}

int actionmoni_counts(const char *_key, uint32_t cs)
{
    int len = 1;
    uint32_t ac = AC_NODE_ADD_CS;
    memcpy(buf_4096 + len, &ac, sizeof(uint32_t));
    len += sizeof(uint32_t);

    uint32_t key = fnv1_32(_key, strlen(_key));
    memcpy(buf_4096 + len, &key, sizeof(uint32_t));
    len += sizeof(uint32_t);

    memcpy(buf_4096 + len, &cs, sizeof(uint32_t));
    len += sizeof(uint32_t);

    return actionmoni_fd > -1
           && sendto(actionmoni_fd, buf_4096, len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
}

int actionmoni_ts(const char *_key, int ts)
{
    int len = 1;
    uint32_t ac = 100 + ts;
    memcpy(buf_4096 + len, &ac, sizeof(uint32_t));
    len += sizeof(uint32_t);

    uint32_t key = fnv1_32(_key, strlen(_key));
    memcpy(buf_4096 + len, &key, sizeof(uint32_t));
    len += sizeof(uint32_t);

    return actionmoni_fd > -1
           && sendto(actionmoni_fd, buf_4096, len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
}

int actionmoni_multi(int cnt, ...)
{
    if(actionmoni_fd == -1) {
        return 0;
    }

    buf_4096[0] = '@';
    int len = 5;

    va_list v1;
    uint32_t i = 0, k = 0;
    uint32_t ac = 0, v = 0;
    char *_key = NULL;
    va_start(v1, cnt);

    while(i < cnt) {
        k++;

        if(k == 1) {
            ac = va_arg(v1, uint32_t);

        } else if(k == 2) {
            _key = va_arg(v1, char *);

            if(!_key) {
                break;
            }

            if(ac != AC_NODE_ADD_CS) {
                k = 0;
                memcpy(buf_4096 + len, "#", 1);
                len += 1;
                memcpy(buf_4096 + len, &ac, sizeof(uint32_t));
                len += sizeof(uint32_t);

                uint32_t key = fnv1_32(_key, strlen(_key));
                memcpy(buf_4096 + len, &key, sizeof(uint32_t));
                len += sizeof(uint32_t);

                i++;
            }

        } else if(k++ >= 3) {
            v = va_arg(v1, uint32_t);
            k = 0;

            memcpy(buf_4096 + len, "#", 1);
            len += 1;

            memcpy(buf_4096 + len, &ac, sizeof(uint32_t));
            len += sizeof(uint32_t);

            uint32_t key = fnv1_32(_key, strlen(_key));
            memcpy(buf_4096 + len, &key, sizeof(uint32_t));
            len += sizeof(uint32_t);

            memcpy(buf_4096 + len, &v, sizeof(uint32_t));
            len += sizeof(uint32_t);

            i++;
        }
    }

    va_end(v1);

    memcpy(buf_4096 + 1, &i, sizeof(uint32_t));

    int n = sendto(actionmoni_fd, buf_4096, len, 0, (struct sockaddr *)&servaddr,
                   sizeof(servaddr));

    buf_4096[0] = '#';

    return n;
}

int actionmoni_set_keys(const char *_keys, int _len)
{
    if(_len > 4080) {
        return 0;
    }

    int len = 1;
    uint32_t ac = AC_SET_KEY_LIST;
    memcpy(buf_4096 + len, &ac, sizeof(uint32_t));
    len += sizeof(uint32_t);

    uint32_t key = _len;
    memcpy(buf_4096 + len, &key, sizeof(uint32_t));
    len += sizeof(uint32_t);

    memcpy(buf_4096 + len, _keys, _len);
    len += _len;
    printf("%d %s\n", actionmoni_fd, buf_4096 + 9);
    return actionmoni_fd > -1
           && sendto(actionmoni_fd, buf_4096, len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
}
