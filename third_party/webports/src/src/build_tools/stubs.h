/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * glibc's <gnu/stubs.h> declares which functions are stubs in glibc
 * but some of these are provided by nacl_io.  autoconf tests will
 * often check for the __stub_XXX macros and so will incorrectly
 * determine that these functions are missing.  This header modifies
 * the effect of the toolchain's stubs.h by undefining the macros
 * for functions that exist in nacl_io.
 */
#include_next <gnu/stubs.h>

#undef __stub_fcntl
#undef __stub_fcntl
#undef __stub_pipe
#undef __stub_select
#undef __stub_socket
#undef __stub_poll
#undef __stub_statvfs
#undef __stub_fstatvfs
#undef __stub_fstatat
#undef __stub_setsid
#undef __stub_connect
#undef __stub_tcgetattr
#undef __stub_tcsetattr
#undef __stub_getrusage
#undef __stub_setrusage
#undef __stub_setpgid
#undef __stub_getpgid
#undef __stub_getgroups
#undef __stub_setgroups
