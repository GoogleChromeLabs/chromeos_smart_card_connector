# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

HOST_BUILD_DIR=${WORK_DIR}/build_host
HOST_INSTALL_DIR=${WORK_DIR}/install_host

export ac_cv_func_getrlimit=no
export EXTRA_AM_CPPFLAGS="-Dmain=nacl_main"
export EXTRA_LIBS="${NACL_CLI_MAIN_LIB} -lppapi_simple \
  -lnacl_io -lppapi -l${NACL_CXX_LIB}"

BuildHostBinutils() {
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  CC="gcc" EXTRA_LIBS="" EXTRA_AM_CPPFLAGS="" \
      LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
      --target=avr \
      --disable-nls
  EXTRA_LIBS="" EXTRA_AM_CPPFLAGS="" LogExecute make
  EXTRA_LIBS="" EXTRA_AM_CPPFLAGS="" LogExecute make install
}

ConfigureStep() {
  ChangeDir ${SRC_DIR}
  BuildHostBinutils
  export PATH="${HOST_INSTALL_DIR}/bin:${PATH}"
  ChangeDir ${BUILD_DIR}
  EXTRA_CONFIGURE_ARGS="\
    --target=avr \
    --disable-nls \
    --disable-werror \
    --enable-deterministic-archives \
    --without-zlib"

  DefaultConfigureStep
}
