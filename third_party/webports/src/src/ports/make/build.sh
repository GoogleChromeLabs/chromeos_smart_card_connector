# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=make${NACL_EXEEXT}

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main -Dpipe=nacl_spawn_pipe"
export LIBS="${NACL_CLI_MAIN_LIB}"

if [ "${NACL_LIBC}" = "newlib" ]; then
  # TODO(sbc): remove once nacl_io implements these.
  export ac_cv_func_getrlimit=no
  export ac_cv_func_seteuid=no
  export ac_cv_func_setegid=no
  # Without this configure will sometimes erroneously define
  # GETLOADAVG_PRIVILEGED=1 (seem to be in the presence of libbsd)
  export ac_cv_func_getloadavg_setgid=no
  NACLPORTS_CPPFLAGS+=" -D_POSIX_VERSION"
fi
