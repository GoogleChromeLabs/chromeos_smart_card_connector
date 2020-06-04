# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="bdftopcf${NACL_EXEEXT}"

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
NACLPORTS_LDFLAGS+=" -Dmain=nacl_main"
export LIBS="-lz ${NACL_CLI_MAIN_LIB}"

EnableGlibcCompat
