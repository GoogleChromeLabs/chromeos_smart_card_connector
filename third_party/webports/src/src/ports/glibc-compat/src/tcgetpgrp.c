/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>

pid_t tcgetpgrp(int fd) {
  errno = ENOTTY;
  return -1;
}

int tcsetpgrp(int fd, pid_t pgrp) {
  errno = ENOTTY;
  return -1;
}
