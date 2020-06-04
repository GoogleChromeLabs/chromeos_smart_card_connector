/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_SYS_UTIME_H
#define GLIBCEMU_SYS_UTIME_H

#include_next <sys/utime.h>

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

int utime(const char *filename, const struct utimbuf *times);
int utimes(const char *filename, const struct timeval times[2]);

#ifdef __cplusplus
};
#endif

#endif /* GLIBCEMU_SYS_UTIME_H */
