# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_CFLAGS+=" -Dmain=nacl_main"
BUILD_DIR=${SRC_DIR}
EXECUTABLES="tree"
NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"
INSTALL_TARGETS="install prefix=${DESTDIR}/${PREFIX}"

BuildStep() {
  LogExecute make clean
  LogExecute make -j${OS_JOBS} CC="${NACLCC}" CFLAGS=" -c \
    ${NACLPORTS_CFLAGS}" LDFLAGS="${NACLPORTS_LDFLAGS}" tree
}
