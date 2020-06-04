# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="fc-cache/fc-cache${NACL_EXEEXT}"
EXTRA_CONFIGURE_ARGS="--disable-docs --with-arch=x86"

# TODO(sbc): Disable use of random(), because it creates the expectation
#            that initstate(), setstate() are implemented too.
#            Remove if initstate() and setstate() are added to libnacl.
if [ ${NACL_LIBC} = "newlib" ]; then
  export ac_cv_func_random=no
fi

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
export LIBS=" ${NACL_CLI_MAIN_LIB}"
