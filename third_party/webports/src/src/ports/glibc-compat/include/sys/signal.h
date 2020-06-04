/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef GLIBCEMU_SYS_SIGNAL_H
#define GLIBCEMU_SYS_SIGNAL_H

#include_next <sys/signal.h>

/*
 * newlib defines SA_SIGINFO even if _POSIX_REALTIME_SIGNALS is not defined (and
 * therefore sa_sighandler is not a member of sigaction).  As it turns out a lot
 * of software assumgins that if SA_SIGINFO is defined then is can/should use
 * sa_sighandler.
 * TODO(sbc): remove this once this bug is fixed:
 * https://code.google.com/p/nativeclient/issues/detail?id=3989
 */
#ifndef _POSIX_REALTIME_SIGNALS
#undef SA_SIGINFO
#endif

#define SA_RESTART 0x10000000

struct sigvec {
  void (*sv_handler)();
  int sv_mask;
  int sv_flags;
};

#endif
