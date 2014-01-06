#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <inttypes.h>
#include <sched.h>
#include <sys/shm.h>
#ifdef  __APPLE__
#include <mach-o/dyld.h>
#endif

#ifndef PROCESS_H
#define PROCESS_H

#define large_malloc(s) (malloc(((int)(s/4096)+1)*4096))

#define cr_printf(fmt, ...) printf("%s%s [%d-%s:%s:%d] " fmt "%s", "\x1b[31m", &now_c, pid, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__, "\x1b[0m")
#define cg_printf(fmt, ...) printf("%s%s [%d-%s:%s:%d] " fmt "%s", "\x1b[32m", &now_c, pid, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__, "\x1b[0m")
#define cy_printf(fmt, ...) printf("%s%s [%d-%s:%s:%d] " fmt "%s", "\x1b[33m", &now_c, pid, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__, "\x1b[0m")
#define cb_printf(fmt, ...) printf("%s%s [%d-%s:%s:%d] " fmt "%s", "\x1b[34m", &now_c, pid, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__, "\x1b[0m")
#define cm_printf(fmt, ...) printf("%s%s [%d-%s:%s:%d] " fmt "%s", "\x1b[35m", &now_c, pid, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__, "\x1b[0m")
#define cc_printf(fmt, ...) printf("%s%s [%d-%s:%s:%d] " fmt "%s", "\x1b[0m", &now_c, pid, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__, "\x1b[0m")

extern char **environ;
const char **_argv;
int _argc;

int get_cpu_num();
int set_cpu_affinity(uint32_t active_cpu);
void set_process_title(const char *title, int is_master);
char *init_process_title(int argc, const char **argv);
void start_master_main(void (*func)(), void (*onexit)());
int check_process_for_exit();
void daemonize();
void attach_on_exit(void *fun);
void set_process_user(const char *user, const char *group);
char *getarg(const char *key);
int fork_process(void (*func)(int i));
void safe_process();
void wait_for_child_process_exit();
int new_thread_p(void *func, void *i);
#endif
