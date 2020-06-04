# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="toybox"

# Toybox wants to build in its current directory.
BUILD_DIR=${SRC_DIR}

NACLPORTS_CPPFLAGS+=" -DBYTE_ORDER=LITTLE_ENDIAN"
NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
NACLPORTS_CPPFLAGS+=" -Dpipe=nacl_spawn_pipe"
NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"

export HOSTCC=cc

EnableGlibcCompat

NACLPORTS_LDFLAGS+=" -l${NACL_CXX_LIB}"

ConfigureStep() {
  LogExecute cp ${START_DIR}/toybox.config ${SRC_DIR}/.config
}

BuildStep() {
  # We can't use NACL_CROSS_PREFIX without also redefining the CC and HOSTCC
  # variables.
  if [[ "${NACLCC}" = *clang ]]; then
    CC=clang
  else
    CC=gcc
  fi

  export CROSS_COMPILE="${NACL_CROSS_PREFIX}-"
  export LDFLAGS="${NACLPORTS_LDFLAGS}"
  export CFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS}"
  export CC
  LogExecute make clean
  DefaultBuildStep
}

InstallStep() {
  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR="${PUBLISH_DIR}/toybox"
  MakeDir ${ASSEMBLY_DIR}

  cp ${BUILD_DIR}/toybox ${ASSEMBLY_DIR}/toybox_${NACL_ARCH}${NACL_EXEEXT}

  ChangeDir ${ASSEMBLY_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${ASSEMBLY_DIR}/toybox_*${NACL_EXEEXT} \
      -s . \
      -o toybox.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py toybox.nmf

  InstallNaClTerm ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/manifest.json ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_16.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_48.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_128.png ${ASSEMBLY_DIR}
  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip toybox-${VERSION}.zip toybox
}
