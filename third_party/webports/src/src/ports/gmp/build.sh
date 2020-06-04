# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

HOST_BUILD_DIR=${WORK_DIR}/build_host
HOST_INSTALL_DIR=${WORK_DIR}/install_host

BuildHostGmp() {
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  CC="gcc" \
      LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR}
  LogExecute make
  LogExecute make install
}

if [ "${NACL_SHARED}" = "1" ]; then
  NACLPORTS_CFLAGS+=" -fPIC"
fi

# define LONGLONG_STANDALONE so that longlong.h doesn't
# contain references to machine-dependant functions.
# This is needed in particular on ARM but is correct on
# all platforms since we don't compile machine-depenedant
# files.
NACLPORTS_CFLAGS+=" -DLONGLONG_STANDALONE"

# Disable all assembly code by specifying none-none-none.
EXTRA_CONFIGURE_ARGS=--host=none-none-none

ConfigureStep() {
  if [ "${OS_NAME}" = "Linux" ]; then
    ChangeDir ${SRC_DIR}
    BuildHostGmp
    ChangeDir ${BUILD_DIR}
  fi
  DefaultConfigureStep
}

TestStep() {
  MAKE_TARGETS="allprogs"
  EXECUTABLES="demos/calc/calc${NACL_EXEEXT}"
  ChangeDir ${BUILD_DIR}/demos
  DefaultBuildStep
  RunPostBuildStep
  if [ "${NACL_ARCH}" != "pnacl" ]; then
    # Perform a simple calculation using the calc demo program
    RESULT=$(echo "1000 * 1000" | demos/calc/calc)
    if [ "$RESULT" != "1000000" ]; then
      echo "Unexpted result from calc test: ${RESULT}"
      exit 1
    fi
  fi
}
