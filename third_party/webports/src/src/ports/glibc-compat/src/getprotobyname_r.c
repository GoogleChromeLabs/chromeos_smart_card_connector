/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <netdb.h>

struct protoent *getprotobyname(const char *name) {
  return NULL;
}

int __getprotobyname_r(const char *name,
                       struct protoent *result_buf, char *buf,
                       size_t buflen, struct protoent **result)
{
  *result = NULL;
  return 1;
}
