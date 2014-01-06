#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

#include "common/process.h"
#include "common/network.h"
#include "common/times.h"
#include "common/timeouts.h"
#include "common/smp.h"
#include "common/log.h"
#include "common/actionmoni-client.h"
#include "common/is-binary.h"
#include "common/base64.h"
#include "common/urlcoder.h"
#include "common/strings.h"
#include "common/mime.h"
#include "se/se.h"

#ifndef _MERRY_H
#define _MERRY_H

extern time_t now;
extern struct tm _now_gtm;
extern struct tm _now_lc;
extern char now_gmt[32];
extern char now_lc[32];

extern int LOG_FD;
extern int LOG_LEVEL;
extern char *log_buf;
extern long *log_buf_len;

extern char hostname[1024];
extern char process_chdir[924];
extern char process_name[100];
extern int is_daemon;
extern int process_count;
extern int pid;

extern int server_fd;
extern int loop_fd;

extern char bind_addr[20];
extern int bind_port;
extern const char *program_name;

int merry_start(int argc, const char **argv, void (*help)(), void (*master)(),
                void (*onexit)(), void (*worker)());

#endif