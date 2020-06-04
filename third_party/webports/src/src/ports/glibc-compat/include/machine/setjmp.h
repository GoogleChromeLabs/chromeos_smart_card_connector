/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_MACHINE_SETJMP_H
#define GLIBCEMU_MACHINE_SETJMP_H 1

/*
 * Includes the newlib version of machine/setjmp.h but define __rtems__
 * so that sigjmp_buf is defined.
 * TODO(sbc): remove this file once newlib's setjmp.h is updated
 *
 * Undefine _POSIX_THREADS so that pthread_sigmask is not used.
 * TODO(sbc): remove this once pthread_sigmask exists in libnacl:
 * https://codereview.chromium.org/642873002/
 */

#include <sys/types.h>
#include <sys/signal.h>
#undef _POSIX_THREADS
#define __rtems__
#include_next <machine/setjmp.h>
#undef __rtems__
#define _POSIX_THREADS

#endif
