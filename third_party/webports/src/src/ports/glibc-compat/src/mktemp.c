/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

/*
 * Quick a dirty version of mkdtemp. This is needed since the one
 * compiled into newlib is currently non-functional (it returns the
 * same value each times its called).
 * TODO(sbc): remove this file once we fix:
 * https://code.google.com/p/nativeclient/issues/detail?id=4020
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char* _mktemp(char *template, int suffixlen, int* fd, int open_flags) {
  int i;
  int len = strlen(template);
  if (len < 6) {
    errno = EINVAL;
    return NULL;
  }

  const char* suffix = "";
  if (suffixlen)
    suffix = template + len - suffixlen;

  /* template_start points to the start of the XXXXXX string */
  char* template_start = template + len - suffixlen - 6;

  for (i = 0; i < 6; i++) {
    if (template_start[i] != 'X') {
      errno = EINVAL;
      return NULL;
    }
  }

  for (i = 1; INT_MAX; i++) {
    /* Write the 6 digit number plus the suffix */
    sprintf(template_start, "%06d%s", i, suffix);
    struct stat buf;
    if (stat(template, &buf) == -1 && errno == ENOENT) {
      if (fd) {
        *fd = open(template, open_flags, 0600);
        if (*fd != -1) {
          return template;
        }
      }
      return template;
    }
  }

  /* Should never get here. */
  assert(0);
  abort();
}

char *mkdtemp(char *template) {
  char* rtn = _mktemp(template, 0, NULL, 0);
  if (rtn == NULL)
    return NULL;

  if (mkdir(template, 0700) != 0)
    return NULL;

  return template;
}

char *mktemp(char *template) {
  if (_mktemp(template, 0, NULL, 0) == NULL)
    template[0] = '\0';
  return template;
}

int mkstemp(char *template) {
  int fd = -1;
  if (_mktemp(template, 0, &fd, O_RDWR | O_EXCL | O_CREAT) == NULL)
    return -1;
  return fd;
}

int mkstemps(char *template, int suffixlen) {
  int fd = -1;
  if (_mktemp(template, suffixlen, &fd, O_RDWR | O_EXCL | O_CREAT) == NULL)
    return -1;
  return fd;
}

int mkostemp(char *template, int flags) {
  int fd = -1;
  if (_mktemp(template, 0, &fd, flags) == NULL)
    return -1;
  return fd;
}

int mkostemps(char *template, int suffixlen, int flags) {
  int fd = -1;
  if (_mktemp(template, suffixlen, &fd, flags) == NULL)
    return -1;
  return fd;
}
