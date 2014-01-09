#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#ifndef _LOG_H
#define _LOG_H

#define DEBUG 1
#define INFO 2
#define NOTICE 3
#define WARN 4
#define ALERT 5
#define ERR 6

#define LOGF(l,a,...) if(l>=LOG_LEVEL&&((is_daemon==1||printf("%s%s [%d-%s:%s:%d] %s " a "%s", \
                                        (l==DEBUG?"\x1b[0m":(l==INFO?"\x1b[32m":(l==NOTICE?"\x1b[34m":(l==WARN?"\x1b[33m":(l==ALERT?"\x1b[35m":"\x1b[31m"))))), \
                                        now_lc, pid, __FILE__, __FUNCTION__, __LINE__, \
                                        (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
                                        ##__VA_ARGS__, "\x1b[0m\n") ))&&LOG_FD>-1) \
    log_writef("%s [%d-%s:%s:%d] %s "a"\n", now_lc, pid, __FILE__, __FUNCTION__, __LINE__, \
               (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
               ##__VA_ARGS__)

#define _LOGF(l,f,a,...) if(l>=LOG_LEVEL&&((is_daemon==1||printf("%s%s [%d-%s] %s " a "%s", \
                            (l==DEBUG?"\x1b[0m":(l==INFO?"\x1b[32m":(l==NOTICE?"\x1b[34m":(l==WARN?"\x1b[33m":(l==ALERT?"\x1b[35m":"\x1b[31m"))))), \
                            now_lc, pid, f, \
                            (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
                            ##__VA_ARGS__, "\x1b[0m\n") ))&&LOG_FD>-1) \
    log_writef("%s [%d-%s] %s "a"\n", now_lc, pid, f, \
               (l==DEBUG?"DEBUG":(l==INFO?"INFO":(l==NOTICE?"NOTICE":(l==WARN?"WARN":(l==ALERT?"ALERT":"ERR"))))), \
               ##__VA_ARGS__)

int open_log(const char *fn, int sz);
void log_destory();
int log_writef(const char *fmt, ...);
void copy_buf_to_shm_log_buf();
int sync_logs();

#endif
