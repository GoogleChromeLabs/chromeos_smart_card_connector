# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export ac_cv_func_sigaction=yes
NACLPORTS_LIBS+=" ${NACL_CLI_MAIN_LIB}"
NACLPORTS_CFLAGS+=" -Dmain=nacl_main"

EnableGlibcCompat

if [ "${NACL_ARCH}" = "pnacl" ]; then
  export gl_cv_header_wchar_h_correct_inline=yes
fi
