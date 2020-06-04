/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

// xgetcwd is pulled from toybox lib/xwrap.c
static char *xgetcwd(void) {
  char *buf = (char*)malloc(sizeof(char) * (PATH_MAX+1));
  if (buf == NULL) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  buf = getcwd(buf, PATH_MAX+1);
  if (!buf) {
    perror("xgetcwd");
    exit(1);
  }
  return buf;
}

// The following functions are pulled from toybox nacl.patch
#define _AT_WRAP_START(A)                             \
  int fchdir_err = 0;                                 \
  char *save = xgetcwd();                             \
  if (!save) {                                        \
    perror("fd_wrapper_"A);                           \
    exit(1);                                          \
  }                                                   \
  if (dirfd != AT_FDCWD) {                            \
    fchdir_err = fchdir(dirfd);                       \
    if (fchdir_err == -1)                             \
      perror("fchdir");                               \
  }

#define _AT_WRAP_END(A)                 \
  if (dirfd != AT_FDCWD) chdir(save);   \
  free(save);

int openat(int dirfd, const char *pathname, int flags, ...) {
  _AT_WRAP_START("openat")
  int fd = open(pathname, flags);
  _AT_WRAP_END("openat")
  return fd;
}

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) {
  // We are going to ignore flags here.
  //if (flags) perror_exit("fstatat_flags");
  _AT_WRAP_START("fstatat")
  int result;
  if (flags & AT_SYMLINK_NOFOLLOW)
    result = lstat(pathname, buf);
  else
    result = stat(pathname, buf);
  _AT_WRAP_END("fstatat")
  return result;
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
  // We are going to ignore flags here.
  //if (flags) perror_exit("fchmodat_flags");
  _AT_WRAP_START("fchmodat")
  int result = chmod(pathname, mode);
  _AT_WRAP_END("fchmodat")
  return result;
}

int readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
  _AT_WRAP_START("readlinkat")
  int result = readlink(pathname, buf, bufsiz);
  _AT_WRAP_END("readlinkat")
  return result;
}

int unlinkat(int dirfd, const char *pathname, int flags) {
  // We are going to ignore flags here.
  //if (flags) perror_exit("unlinkat_flags");
  _AT_WRAP_START("unlinkat")
  int result;
  if(flags & AT_REMOVEDIR) {
    result = rmdir(pathname);
  } else {
    result = unlink(pathname);
  }
  _AT_WRAP_END("unlinkat")
  return result;
}

int faccessat(int dirfd, const char *pathname, int mode, int flags) {
  //if (flags) perror_exit("faccessat_flags");
  _AT_WRAP_START("faccessat")
  int result = access(pathname, mode);
  _AT_WRAP_END("faccessat")
  return result;
}

DIR *fdopendir(int dirfd) {
  _AT_WRAP_START("fdopendir")
  DIR *dir;
  if (fchdir_err) {
    perror("fdopendir: ");
  }
  dir = opendir(".");
  _AT_WRAP_END("fdopendir")
  return dir;
}

int mkdirat(int dirfd, const char *pathname, mode_t mode) {
  _AT_WRAP_START("mkdirat")
  int result = mkdir(pathname, mode);
  _AT_WRAP_END("mkdirat")
  return result;
}

int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) {
  fprintf(stderr, "mknod not supported\n");
  return 1;
}

int fchownat(int dirfd, const char *pathname, uid_t owner,
    gid_t group, int flags) {
  _AT_WRAP_START("fchownat")
  int result = chown(pathname, owner, group);
  _AT_WRAP_END("fchownat")
  return result;
}

int symlinkat(const char *oldpath, int dirfd, const char *newpath) {
  _AT_WRAP_START("symlinkat")
  int result = symlink(oldpath, newpath);
  _AT_WRAP_END("symlinkat")
  return result;
}

int linkat(int olddirfd, const char *oldpath,
    int newdirfd, const char *newpath, int flags) {
  int result;
  if ((oldpath[0] == '/') && (newpath[0] == '/')) {
    result = link(oldpath, newpath);
  } else {
    errno = EINVAL;
    result = -1;
  }
  // We do not support double linking.
  return result;
}
