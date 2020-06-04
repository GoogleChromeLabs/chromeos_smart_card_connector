/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include_next <stdlib.h>

#ifndef GLIBCEMU_STDLIB_H
#define GLIBCEMU_STDLIB_H 1

#include <sys/cdefs.h>

__BEGIN_DECLS

long int random(void);

void srandom(unsigned int seed);

extern void qsort_r(void *base, size_t nmemb, size_t size,
                    int (*compar)(const void *, const void *, void *),
                    void *arg);

__END_DECLS

#endif
