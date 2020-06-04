# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACL_CONFIGURE_PATH=${SRC_DIR}/unix/configure

NACLPORTS_CPPFLAGS="-Dmain=nacl_main"

export LIBS="${NACL_CLI_MAIN_LIB}"

MAKE_TARGETS="binaries"
INSTALL_TARGETS="install-binaries"

# Disable fallbacks for broken libc's that kick in for
# cross-compiles since autoconf can't run target binaries.
# The fallbacks seem to be non-general.
export tcl_cv_strtod_buggy=ok
export ac_cv_func_strtod=yes
export ac_cv_func_memmove=yes
export tcl_cv_strtod_unbroken=ok

# Prevent non-cross compile clean parts of the build from assuming the host
# system influences things (needed for OSX).
export tcl_cv_sys_version=Generic

if [ "${NACL_SHARED}" = "1" ]; then
  NACLPORTS_CFLAGS+=" -fPIC"
fi

EnableGlibcCompat

if [ "${NACL_LIBC}" = "newlib" ]; then
  NACLPORTS_CPPFLAGS+=" -DHAVE_STRLCPY=1"
  EXTRA_CONFIGURE_ARGS+=" --enable-shared=no"
  EXTRA_CONFIGURE_ARGS+=" --enable-load=no"
  export tcl_cv_strtoul_unbroken=ok
fi

if [ "${NACL_LIBC}" = "bionic" ]; then
  NACLPORTS_CPPFLAGS+=" -DHAVE_STRLCPY=1"
fi
