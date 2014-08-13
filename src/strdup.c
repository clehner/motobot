/*
 * strdup.c
 * Copyright (c) 2014 Charles Lehner
 */

#include <stdlib.h>
#include <string.h>
#include "strdup.h"

char *my_strdup(const char *s) {
	size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p) strncpy(p, s, len);
    return p;
}
