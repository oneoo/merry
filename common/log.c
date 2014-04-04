#include "log.h"
#include "times.h"
#include <errno.h>
logf_t *LOGF_T = NULL;
int LOG_LEVEL = NOTICE;

extern time_t now;
extern struct tm _now_gtm;
extern struct tm _now_lc;
extern char now_gmt[32];
extern char now_lc[32];

extern int is_daemon;
extern int pid;

static char buf_4096[4096] = {0};

logf_t *open_log(const char *fn, int sz)
{
    if(!fn) {
        return NULL;
    }

    char *p = strstr(fn, ",");
    char oldc = 0;

    if(p) {
        if(stristr(p, "DEBUG")) {
            LOG_LEVEL = 1;

        } else if(stristr(p, "INFO")) {
            LOG_LEVEL = 2;

        } else if(stristr(p, "NOTICE")) {
            LOG_LEVEL = 3;

        } else if(stristr(p, "WARN")) {
            LOG_LEVEL = 4;

        } else if(stristr(p, "ALERT")) {
            LOG_LEVEL = 5;

        } else if(stristr(p, "ERR")) {
            LOG_LEVEL = 6;

        } else if(stristr(p, "NO")) {
            LOG_LEVEL = 7;

        } else {
            int i = atoi(p + 1);

            if(i > 0 && i < 8) {
                LOG_LEVEL = i;
            }
        }

        oldc = p[0];
        p[0] = '\0';
    }

    if(!fn || strlen(fn) < 1) {
        p[0] = oldc;
        return NULL;
    }

    int fd = open(fn, O_APPEND | O_CREAT | O_WRONLY, 0644);

    if(p) {
        p[0] = oldc;
    }

    logf_t *_logf = NULL;

    if(fd > -1) {
        _logf = malloc(sizeof(logf_t));

        if(_logf) {
            bzero(_logf, sizeof(logf_t));
            _logf->LOG_FD = fd;
            _logf->_shm_log_buf = shm_malloc(sz + 4);

            if(_logf->_shm_log_buf) {
                _logf->log_buf_len = _logf->_shm_log_buf->p + sz;
                *(_logf->log_buf_len) = 0;
                _logf->log_buf = _logf->_shm_log_buf->p;
                _logf->log_buf_size = sz > 4096 ? sz : 4096;

            } else {
                LOGF(ERR, "malloc error (%s)", strerror(errno));
                free(_logf);
                _logf = NULL;
            }
        }
    }

    return _logf;
}

void log_destory(logf_t *_logf)
{
    if(!_logf) {
        return;
    }

    sync_logs(_logf);
    shm_free(_logf->_shm_log_buf);
    _logf->_shm_log_buf = NULL;

    if(_logf->LOG_FD != -1) {
        close(_logf->LOG_FD);
        _logf->LOG_FD = -1;
    }
}

void copy_buf_to_shm_log_buf(logf_t *_logf)
{
    if(!_logf || _logf->_inner_log_buf_len < 1) {
        return;
    }

    shm_lock(_logf->_shm_log_buf);

    if(*(_logf->log_buf_len) + _logf->_inner_log_buf_len > _logf->log_buf_size) {
        write(_logf->LOG_FD, _logf->log_buf, *(_logf->log_buf_len));
        *(_logf->log_buf_len) = 0;
    }

    memcpy(_logf->log_buf + (*_logf->log_buf_len), _logf->_inner_log_buf, _logf->_inner_log_buf_len);
    *(_logf->log_buf_len) += _logf->_inner_log_buf_len;
    shm_unlock(_logf->_shm_log_buf);

    _logf->_inner_log_buf_len = 0;
}

int log_writef(logf_t *_logf, const char *fmt, ...)
{
    if(!_logf || _logf->LOG_FD == -1) {
        return 0;
    }

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf_4096, 4096, fmt, args);
    va_end(args);

    update_time();

    if(now > _logf->last_wtime || _logf->_inner_log_buf_len + n > 4096) {
        _logf->last_wtime = now;
        copy_buf_to_shm_log_buf(_logf);
    }

    memcpy(_logf->_inner_log_buf + _logf->_inner_log_buf_len, buf_4096, n);
    _logf->_inner_log_buf_len += n;

    return n;
}

int sync_logs(logf_t *_logf)
{
    if(!_logf || _logf->LOG_FD == -1) {
        return 0;
    }

    copy_buf_to_shm_log_buf(_logf);
    int n = 0;

    if(*_logf->log_buf_len > 0) {
        n = write(_logf->LOG_FD, _logf->log_buf, *_logf->log_buf_len);
        *_logf->log_buf_len = 0;
    }

    return n;
}
