/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __MYSYSLOG_H__
#define __MYSYSLOG_H__

#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_WARNING 2
#define LOG_ERR 3
#define LOG_CRIT 4
#define LOG_NOTICE 5
#define LOG_DEBUG 5

#ifdef __cplusplus
extern "C" {
#endif
void syslog(int level, const char* message, ...);
void network_error();
#ifdef __cplusplus
}
#endif

#endif  // __MYSYSLOG_H__

