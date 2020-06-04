# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="gettext-tools/src/msgfmt${NACL_EXEEXT}"

EnableGlibcCompat

if [ "${NACL_LIBC}" = "glibc" ]; then
  # The configure test for sigaction detects the fact
  # that glibc contains only stubs for sigation.  We
  # want to ignore this and assume a working sigaction.
  export ac_cv_func_sigaction=yes
fi

if [ "${NACL_LIBC}" = "newlib" ]; then
  export lt_cv_archive_cmds_need_lc=no
fi
