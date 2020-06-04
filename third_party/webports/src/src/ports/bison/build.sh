# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_LIBS+="${NACL_CLI_MAIN_LIB}"
NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"

EnableGlibcCompat

NACLPORTS_CPPFLAGS+=" -DGNULIB_defined_struct_sigaction"
