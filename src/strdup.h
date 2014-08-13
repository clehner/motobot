/*
 * strdup.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef STRDUP_H
#define STRDUP_H

char *my_strdup(const char *s);

#define strdup(x) my_strdup(x)

#endif /* STRDUP_H */
