/* Copyright 2015 The Native Client Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* pull from freebsd 10.1 ./contrib/ldns/compat/timegm.c */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

time_t
timegm (struct tm *tm) {
  time_t ret;
  char *tz;

  tz = getenv("TZ");
  putenv((char*)"TZ=");
  tzset();
  ret = mktime(tm);
  if (tz) {
    char buf[256];
    snprintf(buf, sizeof(buf), "TZ=%s", tz);
    putenv(tz);
  }
  else
    putenv((char*)"TZ");
  tzset();
  return ret;
}
