/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GLIBCEMU_PTHREAD_H
#define GLIBCEMU_PTHREAD_H

#include_next <pthread.h>

/* Newlib doesn't include a fallback for this call. */
#define pthread_attr_setscope(x,y) (-1)

#endif
