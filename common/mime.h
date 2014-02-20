#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "strings.h"
#include "smp.h"

#ifndef _MIME_H
#define _MIME_H

typedef struct {
    char *ext;
    char *type;
    void *next;
} mime_t;

void init_mime_types();
const char *get_mime_type(const char *filename);

#endif
