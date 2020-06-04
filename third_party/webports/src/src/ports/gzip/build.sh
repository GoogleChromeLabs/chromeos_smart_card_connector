# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=gzip${NACL_EXEEXT}

ConfigureStep() {
  EnableGlibcCompat
  NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
  NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"

  export gzip_cv_assembler=no
  export gl_cv_header_wchar_h_correct_inline=yes
  if [ "${NACL_ARCH}" = pnacl ]; then
    NACLPORTS_CFLAGS+=" -std=gnu89"
    export ac_cv_type_signal=int
    export ac_cv_type_off_t=yes
  fi

  DefaultConfigureStep
}
