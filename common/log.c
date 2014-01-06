#include "log.h"
#include "shm.h"
#include "times.h"

int LOG_FD = -1;
int LOG_LEVEL = WARN;

shm_t *_shm_log_buf = NULL;
char *log_buf = NULL;
long *log_buf_len = NULL;
char _inner_log_buf[4096] = {0};
int _inner_log_buf_len = 0;

extern time_t now;
extern struct tm _now_gtm;
extern struct tm _now_lc;
extern char now_gmt[32];
extern char now_lc[32];

static char buf_4096[4096] = {0};

int open_log(const char *fn, int sz)
{
    if(!fn) {
        return -1;
    }

    char *p = strstr(fn, ",");

    if(p) {
        if(strstr(p, "DEBUG")) {
            LOG_LEVEL = 1;

        } else if(strstr(p, "INFO")) {
            LOG_LEVEL = 2;

        } else if(strstr(p, "NOTICE")) {
            LOG_LEVEL = 3;

        } else if(strstr(p, "WARN")) {
            LOG_LEVEL = 4;

        } else if(strstr(p, "ALERT")) {
            LOG_LEVEL = 5;

        } else if(strstr(p, "ERR")) {
            LOG_LEVEL = 6;

        } else {
            int i = atoi(p + 1);

            if(i > 0 && i < 7) {
                LOG_LEVEL = i;
            }
        }

        p[0] = '\0';
    }

    if(!fn || strlen(fn) < 1) {
        return -1;
    }

    if(LOG_FD < 0) {
        LOG_FD = open(fn, O_APPEND | O_CREAT | O_WRONLY, 0644);
    }

    if(LOG_FD > -1 && !_shm_log_buf) {
        _shm_log_buf = shm_malloc(40960 + 4);

        if(_shm_log_buf) {
            log_buf_len = _shm_log_buf->p + 40960;
            *log_buf_len = 0;
            log_buf = _shm_log_buf->p;
        }
    }

    return LOG_FD;
}

void log_destory()
{
    sync_logs();
    shm_free(_shm_log_buf);
    _shm_log_buf = NULL;

    if(LOG_FD != -1) {
        close(LOG_FD);
        LOG_FD = -1;
    }
}

void copy_buf_to_shm_log_buf()
{
    if(_inner_log_buf_len < 1) {
        return;
    }

    shm_lock(_shm_log_buf);

    if((*log_buf_len) + _inner_log_buf_len > 40960) {
        write(LOG_FD, log_buf, *log_buf_len);
        *log_buf_len = 0;
    }

    memcpy(log_buf + (*log_buf_len), _inner_log_buf, _inner_log_buf_len);
    *log_buf_len = (*log_buf_len) + _inner_log_buf_len;
    shm_unlock(_shm_log_buf);

    _inner_log_buf_len = 0;
}

long last_wtime = 0;
int log_writef(const char *fmt, ...)
{
    if(LOG_FD == -1) {
        return 0;
    }

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf_4096, 4096, fmt, args);
    va_end(args);

    update_time();

    if(now > last_wtime || _inner_log_buf_len + n > 4096) {
        last_wtime = now;
        copy_buf_to_shm_log_buf();
    }

    memcpy(_inner_log_buf + _inner_log_buf_len, buf_4096, n);
    _inner_log_buf_len += n;

    return n;
}

int sync_logs()
{
    if(LOG_FD == -1) {
        return 0;
    }

    copy_buf_to_shm_log_buf();
    int n = 0;

    if(*log_buf_len > 0) {
        n = write(LOG_FD, log_buf, *log_buf_len);
        *log_buf_len = 0;
    }

    return n;
}
