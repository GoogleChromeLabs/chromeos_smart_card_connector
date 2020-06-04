/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <netdb.h>

struct servent *
getservbyname(const char *name, const char *proto)
{
  return NULL;
}

int __getservbyname_r(const char *name, const char *proto,
                      struct servent *result_buf, char *buf,
                      size_t buflen, struct servent **result)
{
  *result = NULL;
  return 1;
}
