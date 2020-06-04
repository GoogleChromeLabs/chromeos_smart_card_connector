/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_TIME_H
#define GLIBCEMU_TIME_H 1

#include_next <time.h>

#include <sys/cdefs.h>

__BEGIN_DECLS
time_t timegm(struct tm *tm);
time_t timelocal(struct tm *tm);
__END_DECLS

#endif
