/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GLIBCEMU_SYS_FCNTL_H
#define GLIBCEMU_SYS_FCNTL_H 1

#include_next <sys/fcntl.h>

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

#define F_ULOCK 0
#define F_LOCK  1
#define F_TLOCK 2
#define AT_EACCESS              1
#define AT_SYMLINK_NOFOLLOW     2
#define AT_SYMLINK_FOLLOW       4
#define AT_REMOVEDIR            8

#if __BSD_VISIBLE
/* lock operations for flock(2) */
#define LOCK_SH         0x01            /* shared file lock */
#define LOCK_EX         0x02            /* exclusive file lock */
#define LOCK_NB         0x04            /* don't block when locking */
#define LOCK_UN         0x08            /* unlock file */
#endif

int lockf(int fd, int command, off_t size);
int flock(int fd, int operation);

__END_DECLS

#endif
