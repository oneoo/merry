#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include "shm.h"
#include "smp.h"

#ifndef _LOG_H
#define _LOG_H

#define DEBUG 1
#define INFO 2
#define NOTICE 3
#define WARN 4
#define ALERT 5
#define ERR 6

typedef struct {
    int LOG_FD;

    shm_t *_shm_log_buf;
    char *log_buf;
    long *log_buf_len;
    char _inner_log_buf[4096];
    int _inner_log_buf_len;
    int log_buf_size;
    long last_wtime;
} logf_t;

#define LOGF(l,a,...) if(l>=LOG_LEVEL&&((is_daemon==1||printf("%s%s [%d-%s:%s:%d] %s " a "%s", \
                                        (l==DEBUG?"\x1b[0m":(l==INFO?"\x1b[32m":(l==NOTICE?"\x1b[34m":(l==WARN?"\x1b[33m":(l==ALERT?"\x1b[35m":"\x1b[31m"))))), \
                                        now_lc, pid, __FILE__, __FUNCTION__, __LINE__, \
                                        (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
                                        ##__VA_ARGS__, "\x1b[0m\n") ))&&LOGF_T) \
    log_writef(LOGF_T, "%s [%d-%s:%s:%d] %s "a"\n", now_lc, pid, __FILE__, __FUNCTION__, __LINE__, \
               (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
               ##__VA_ARGS__)

#define _LOGF(l,f,a,...) if(l>=LOG_LEVEL&&((is_daemon==1||printf("%s%s [%d-%s] %s " a "%s", \
                            (l==DEBUG?"\x1b[0m":(l==INFO?"\x1b[32m":(l==NOTICE?"\x1b[34m":(l==WARN?"\x1b[33m":(l==ALERT?"\x1b[35m":"\x1b[31m"))))), \
                            now_lc, pid, f, \
                            (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
                            ##__VA_ARGS__, "\x1b[0m\n") ))&&LOGF_T) \
    log_writef(LOGF_T, "%s [%d-%s] %s "a"\n", now_lc, pid, f, \
               (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
               ##__VA_ARGS__)

logf_t *open_log(const char *fn, int sz);
void log_destory(logf_t *logf);
int log_writef(logf_t *logf, const char *fmt, ...);
void copy_buf_to_shm_log_buf(logf_t *logf);
int sync_logs(logf_t *logf);

#endif
