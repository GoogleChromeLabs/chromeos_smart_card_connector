/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _GLIBCEMU_SYS_SELECT_H
#define _GLIBCEMU_SYS_SELECT_H 1

#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int select (int __nfds, fd_set *__restrict __readfds,
                   fd_set *__restrict __writefds,
                   fd_set *__restrict __exceptfds,
                   struct timeval *__restrict __timeout);

#ifdef __cplusplus
}
#endif

#endif /* _GLIBCEMU_SYS_SELECT_H */
