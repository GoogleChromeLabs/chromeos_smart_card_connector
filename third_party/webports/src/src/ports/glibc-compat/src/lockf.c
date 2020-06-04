/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>

int lockf(int fd, int command, off_t size) {
  struct flock params;
  bzero(&params, sizeof(params));

  params.l_whence = SEEK_CUR;
  params.l_start = 0;
  params.l_len = size;
  int fcntl_command = F_SETLK;

  switch(command) {
    case F_ULOCK:
      params.l_type = F_UNLCK;
      fcntl_command = F_SETLK;
      break;
    case F_LOCK:
      fcntl_command = F_SETLKW;
    case F_TLOCK:
      params.l_type = F_WRLCK;
      break;
    default:
      errno = EINVAL;
      return -1;
  }

  return fcntl(fd, fcntl_command, &params);
}
