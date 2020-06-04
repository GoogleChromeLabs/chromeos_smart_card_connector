# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACL_CONFIGURE_PATH=${SRC_DIR}/unix/configure

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
NACLPORTS_LIBS+=" -lX11 -lxcb -lXau ${NACL_CLI_MAIN_LIB}"

EXTRA_CONFIGURE_ARGS+=" --with-tcl=${NACLPORTS_LIBDIR}"

if [ "${NACL_ARCH}" = "pnacl" ]; then
  EXTRA_CONFIGURE_ARGS+=" --disable-load"
fi

# Disable fallbacks for broken libc's that kick in for
# cross-compiles since autoconf can't run target binaries.
# The fallbacks seem to be non-general.
export tcl_cv_strtod_buggy=ok

EnableGlibcCompat

# Ideally we would only add this flag for newlib builds but
# linking of the shared library currently fails because it
# tries to link libppapi_stub.a which is not built with -fPIC.
EXTRA_CONFIGURE_ARGS+=" --disable-shared"
