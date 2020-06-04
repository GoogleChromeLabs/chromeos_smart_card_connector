# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# NOTE: This build doesn't work in parallel.
OS_JOBS=1

# This build relies on certain host binaries.
HOST_BUILD_DIR=${WORK_DIR}/build_host
HOST_INSTALL_DIR=${WORK_DIR}/install_host

EXECUTABLES="gforth-ditc${NACL_EXEEXT}"
MAKE_TARGETS="${EXECUTABLES}"

BuildHostGforth() {
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  CC="gcc -m32" \
      LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR}
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  ChangeDir ${SRC_DIR}
  ./autogen.sh
  BuildHostGforth
  export PATH="${HOST_INSTALL_DIR}/bin:${PATH}"
  export skipcode=no
  NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
  export LIBS+=" -Wl,--undefined=nacl_main ${NACL_CLI_MAIN_LIB} \
      -ltar -lppapi_simple -lnacl_io -lppapi -l${NACL_CXX_LIB}"
  EnableGlibcCompat
  ChangeDir ${BUILD_DIR}
  EXTRA_CONFIGURE_ARGS="--without-check"
  DefaultConfigureStep
}

BuildStep() {
  rm -f gforth
  DefaultBuildStep
  cp ${MAKE_TARGETS} gforth
}

InstallStep() {
  MakeDir ${PUBLISH_DIR}
  ChangeDir ${PUBLISH_DIR}

  cp ${BUILD_DIR}/gforth-ditc${NACL_EXEEXT} \
    ${PUBLISH_DIR}/gforth_${NACL_ARCH}${NACL_EXEEXT}

  # TODO(bradnelson): Make this nicer.
  local TAR_DIR=${PUBLISH_DIR}/naclports-dummydir
  MakeDir ${TAR_DIR}

  cp -r ${HOST_INSTALL_DIR}/* ${TAR_DIR}/
  rm -rf ${TAR_DIR}/bin
  rm -rf ${TAR_DIR}/share/info
  rm -rf ${TAR_DIR}/share/man
  rm -rf ${TAR_DIR}/include
  rm -f ${TAR_DIR}/lib/gforth/0.7.2/gforth-ditc
  tar cf ${PUBLISH_DIR}/gforth.tar naclports-dummydir
  rm -rf ${TAR_DIR}
  shasum ${PUBLISH_DIR}/gforth.tar > ${PUBLISH_DIR}/gforth.tar.hash

  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      gforth_*${NACL_EXEEXT} \
      -s . \
      -o gforth.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py gforth.nmf

  InstallNaClTerm ${PUBLISH_DIR}

  cp ${START_DIR}/gforth.js ${PUBLISH_DIR}/

  ChangeDir ${PUBLISH_DIR}
  Remove gforth.zip
  LogExecute zip -r gforth.zip .
}
