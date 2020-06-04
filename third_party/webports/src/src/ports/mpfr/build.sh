# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

HOST_BUILD_DIR=${WORK_DIR}/build_host
HOST_INSTALL_DIR=${WORK_DIR}/install_host

BuildHostMpfr() {
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  CC="gcc" \
      LogExecute ${SRC_DIR}/configure \
      --prefix=${HOST_INSTALL_DIR} \
      --with-gmp=${NACL_PACKAGES_BUILD}/gmp/install_host

  LogExecute make
  LogExecute make install
}

if [ "${NACL_SHARED}" = "1" ]; then
  NACLPORTS_CFLAGS+=" -fPIC"
fi

# Disable all assembly code by specifying none-none-none.
EXTRA_CONFIGURE_ARGS=--host=none-none-none

ConfigureStep() {
  if [ "${OS_NAME}" = "Linux" ]; then
    ChangeDir ${SRC_DIR}
    BuildHostMpfr
    ChangeDir ${BUILD_DIR}
  fi
  DefaultConfigureStep
}
