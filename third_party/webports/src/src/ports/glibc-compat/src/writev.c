/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>

int writev(int fd, const struct iovec *iov, int iovcnt) {
  int i = 0, ret = 0, n = 0, len = 0;
  char* base;
  errno = 0;
  while (i < iovcnt) {
    len = iov->iov_len;
    base = iov->iov_base;
    while (len > 0) {
      ret = write(fd, base, len);
      if (ret < 0 && n == 0) return -1;
      if (ret <= 0) return n;
      errno = 0;
      n += ret;
      len -= ret;
      base += ret;
    }
    iov++;
    i++;
  }
  return n;
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  int i = 0, ret = 0, n = 0, len = 0;
  char* base;
  errno = 0;
  while (i < iovcnt) {
    len = iov->iov_len;
    base = iov->iov_base;
    while (len > 0) {
      ret = read(fd, base, len);
      if (ret < 0 && n == 0) return -1;
      if (ret <= 0) return n;
      errno = 0;
      n += ret;
      len -= ret;
      base += ret;
    }
    iov++;
    i++;
  }
  return n;
}
