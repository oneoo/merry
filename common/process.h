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

#include "smp.h"

#ifndef PROCESS_H
#define PROCESS_H

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
void on_process_exit_handler(int sig, siginfo_t *info, void *secret);
void set_process_user(const char *user, const char *group);
char *getarg(const char *key);
int fork_process(void (*func)(int i));
void safe_process();
void wait_for_child_process_exit();
int new_thread_p(void *func, void *i);
#endif
