# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EXECUTABLES=x264${NACL_EXEEXT}
EXTRA_CONFIGURE_ARGS="--enable-static --disable-asm --disable-thread"

if [ "${NACL_SHARED}" = "1" ]; then
  EXTRA_CONFIGURE_ARGS+=" --enable-shared"
fi

ConfigureStep() {
  DefaultConfigureStep
  LogExecute make clean
}
