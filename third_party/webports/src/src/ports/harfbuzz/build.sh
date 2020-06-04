# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="util/hb-shape${NACL_EXEEXT}\
 util/hb-ot-shape-closure${NACL_EXEEXT}"
NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"

EXTRA_CONFIGURE_ARGS+=" --with-gobject --without-icu"
