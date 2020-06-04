/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_SYS_UNISTD_H
#define GLIBCEMU_SYS_UNISTD_H

#include_next <sys/unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

/*
 * TODO(bradnelson): Consider adding sysconf (and this define) to nacl_io.
 * This would allow us to return more useful values.
 * For example nacl_io knows what this one should be.
 *
 * This sysconf parameter is unsupported by newlib, pick a bad value so that
 * we can still build when this is used, and instead hit a runtime error.
 * One port (tcl) has a fallback in this case to guess a reasonable value.
 */
#define _SC_GETPW_R_SIZE_MAX (-1)
#define _SC_GETGR_R_SIZE_MAX (-1)
#define _SC_MONOTONIC_CLOCK (-1)
#define _SC_OPEN_MAX (-1)
#define _SC_CLK_TCK (-1)
#define _SC_ARG_MAX (-1)

/* TODO(bradnelson): Drop this once we've landed nacl_io pipe support. */
extern int mknod(const char *pathname, mode_t mode, dev_t dev);
extern int mkfifo(const char *pathname, mode_t mode);

int readlinkat(int dirfd, const char *pathname,
                      char *buf, size_t bufsiz);
int unlinkat(int dirfd, const char *pathname, int flags);
int faccessat(int dirfd, const char *pathname, int mode, int flags);
int fchownat(int dirfd, const char *pathname,
                    uid_t owner, gid_t group, int flags);
int symlinkat(const char *oldpath, int newdirfd, const char *newpath);
int linkat(int olddirfd, const char *oldpath,
           int newdirfd, const char *newpath, int flags);

#endif
