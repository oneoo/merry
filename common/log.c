#include "log.h"
#include "times.h"
#include <errno.h>
#include <math.h>
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
    char split_by = 0;
    short auto_delete = 0;

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

        } else if(strstr(p, "NO")) {
            LOG_LEVEL = 7;

        } else {
            int i = atoi(p + 1);

            if(i > 0 && i < 8) {
                LOG_LEVEL = i;
            }
        }

        if(strstr(p, ",h")) {
            split_by = 'h';

        } else if(strstr(p, ",d")) {
            split_by = 'd';

        } else if(strstr(p, ",w")) {
            split_by = 'w';

        } else if(strstr(p, ",m")) {
            split_by = 'm';
        }

        if(split_by != 0 && strstr(p + 1, ",")) {
            auto_delete = atoi(strstr(p + 1, ",") + 2);
        }

        oldc = p[0];
        p[0] = '\0';
    }

    if(!fn || strlen(fn) < 1) {
        p[0] = oldc;
        return NULL;
    }

    int fd = open(fn, O_APPEND | O_CREAT | O_WRONLY, 0644);

    logf_t *_logf = NULL;

    if(fd > -1) {
        _logf = malloc(sizeof(logf_t));

        if(_logf) {
            bzero(_logf, sizeof(logf_t));
            _logf->file = malloc(strlen(fn));

            if(_logf->file) {
                memcpy(_logf->file, fn, strlen(fn));
                _logf->file[strlen(fn)] = '\0';
            }

            _logf->split_by = split_by;
            _logf->auto_delete = auto_delete;
            localtime_r(&now, &_logf->last_split_tm);
            _logf->LOG_FD = fd;
            _logf->_shm_log_buf = shm_malloc(sz + sizeof(long));

            if(_logf->_shm_log_buf) {
                _logf->log_buf_len = (long *)((char *)_logf->_shm_log_buf->p + sz);
                *(_logf->log_buf_len) = 0;
                _logf->log_buf = _logf->_shm_log_buf->p;
                _logf->log_buf_size = sz > LOG_BUF_SIZE * 2 ? sz : LOG_BUF_SIZE * 2;

            } else {
                free(_logf);
                _logf = NULL;
            }
        }
    }

    if(p) {
        p[0] = oldc;
    }

    return _logf;
}

void log_destory(logf_t *_logf)
{
    if(!_logf) {
        return;
    }

    sync_logs(_logf);
    free(_logf->file);
    _logf->file = NULL;
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

    if(now > _logf->last_wtime || _logf->_inner_log_buf_len + n >= LOG_BUF_SIZE) {
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

    if(_logf->split_by) {
        int dosplit = 0;
        time_t old = now;

        if(_logf->split_by == 'h' && _logf->last_split_tm.tm_hour != _now_lc.tm_hour) {
            dosplit = 1;
            old -= _logf->auto_delete * 3600;

        } else if(_logf->split_by == 'd' && _logf->last_split_tm.tm_mday != _now_lc.tm_mday) {
            dosplit = 1;
            old -= _logf->auto_delete * 86400;

        } else if(_logf->split_by == 'w' && _logf->last_split_tm.tm_wday != _now_lc.tm_wday) {
            dosplit = 1;
            old -= _logf->auto_delete * 604800;

        } else if(_logf->split_by == 'm' && _logf->last_split_tm.tm_mon != _now_lc.tm_mon) {
            dosplit = 1;
            old -= _logf->auto_delete * 2592000;

        }

        if(dosplit && _logf->file) {
            sprintf(buf_4096, "%s-%04d-%02d-%02d-%02d", _logf->file, _now_lc.tm_year + 1900, _now_lc.tm_mon + 1, _now_lc.tm_mday,
                    _now_lc.tm_hour);

            if(rename(_logf->file, buf_4096) == 0) {
                close(_logf->LOG_FD);
                localtime_r(&now, &_logf->last_split_tm);
                _logf->LOG_FD = open(_logf->file, O_APPEND | O_CREAT | O_WRONLY, 0644);
            }

            if(_logf->auto_delete > 0) {
                struct tm _old_lc = {0};
                localtime_r(&old, &_old_lc);
                sprintf(buf_4096, "rm -f %s-%04d-%02d-%02d-%02d &", _logf->file, _old_lc.tm_year + 1900, _old_lc.tm_mon + 1,
                        _old_lc.tm_mday, _old_lc.tm_hour);
                FILE *fp = popen(buf_4096, "r");
                pclose(fp);
            }
        }
    }

    copy_buf_to_shm_log_buf(_logf);
    int n = 0;

    if(*_logf->log_buf_len > 0) {
        n = write(_logf->LOG_FD, _logf->log_buf, *_logf->log_buf_len);
        *_logf->log_buf_len = 0;
    }

    return n;
}
