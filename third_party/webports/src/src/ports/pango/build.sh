# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="pango-view/pango-view${NACL_EXEEXT}"
EXTRA_CONFIGURE_ARGS+=" --with-included-modules --without-dynamic-modules"
EXTRA_CONFIGURE_ARGS+=" --with-x"

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"
