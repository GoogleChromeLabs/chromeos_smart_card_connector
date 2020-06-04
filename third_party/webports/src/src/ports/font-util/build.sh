# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The host build of this tool is needed by its dependents.
HOST_BUILD_DIR=${WORK_DIR}/build_host
HOST_INSTALL_DIR=${WORK_DIR}/install_host

EXECUTABLES="bdftruncate${NACL_EXEEXT} ucs2any${NACL_EXEEXT}"

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
NACLPORTS_LDFLAGS+=" -Dmain=nacl_main"
export LIBS="${NACL_CLI_MAIN_LIB}"

EnableGlibcCompat

BuildForHost() {
  Banner "Building for Host"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  CFLAGS="" LDFLAGS="" LIBS="" CC="gcc -m32" \
      LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR}
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  BuildForHost
  Banner "Building for NaCl"
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}
