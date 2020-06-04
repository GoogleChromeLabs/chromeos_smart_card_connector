# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  EnableGlibcCompat
  NACLPORTS_LIBS+=" -l${NACL_CXX_LIB}"
  NACLPORTS_CPPFLAGS+=" -Dpipe=nacl_spawn_pipe"

  # Grep fails to build NDEBUG defined
  # ib/chdir-long.c:62: error: unused variable 'close_fail'
  NACLPORTS_CFLAGS="${NACLPORTS_CFLAGS/-DNDEBUG/}"
  DefaultConfigureStep
}
