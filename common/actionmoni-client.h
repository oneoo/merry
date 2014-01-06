#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>

#ifndef _ACTIONMONI_CLIENT
#define _ACTIONMONI_CLIENT

#define AC_NODE_ADD 5
#define AC_NODE_ADD_CS 11
#define AC_SET_KEY_LIST 98

int actionmoni_open(const char *host, int port);
int actionmoni_count(const char *_key);
int actionmoni_counts(const char *_key, uint32_t cs);
int actionmoni_ts(const char *_key, int ts);
int actionmoni_multi(int cnt, ...);
int actionmoni_set_keys(const char *_keys, int _len);

#endif
