/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GLIBCEMU_SYS_STAT_H
#define GLIBCEMU_SYS_STAT_H 1

#include_next <sys/stat.h>
                                                        /* 7777 */
#define ALLPERMS        (S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
#ifdef __cplusplus
extern "C" {
#endif

mode_t umask(mode_t mask);
int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
int mkdirat(int dirfd, const char *pathname, mode_t mode);
int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev);

#ifdef __cplusplus
}
#endif

#endif
