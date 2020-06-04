# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EXECUTABLES=src/nethack

BuildStep() {
  SetupCrossEnvironment

  # NOTE: we are using the non-standard vars NACL_CCFLAGS/NACL_LDFLAGS
  # because we are not running ./configure and the Makefile was hacked
  export NACL_CCFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS}"
  export NACL_LDFLAGS="${NACLPORTS_LDFLAGS}"
  export WINTTYLIB="-lncurses ${NACL_CLI_MAIN_LIB}"
  if [ "${NACL_LIBC}" = "newlib" ]; then
    export WINTTYLIB="${WINTTYLIB} -lglibc-compat"
    export NACL_CCFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat"
  fi

  export NACLPORTS_INCLUDE
  export STRNCMPI=1
  cp ${START_DIR}/nethack_pepper.c ${SRC_DIR}/src
  LogExecute bash sys/unix/setup.sh
  LogExecute make
}

InstallStep() {
  ChangeDir ${SRC_DIR}

  make install

  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR="${PUBLISH_DIR}/nethack"
  MakeDir ${ASSEMBLY_DIR}
  cp ${SRC_DIR}/out/games/lib/nethackdir/nethack \
      ${ASSEMBLY_DIR}/nethack_${NACL_ARCH}${NACL_EXEEXT}
  ChangeDir ${SRC_DIR}/out/games
  LogExecute rm ${SRC_DIR}/out/games/lib/nethackdir/nethack
  LogExecute tar cf ${ASSEMBLY_DIR}/nethack.tar lib

  pushd ${ASSEMBLY_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      nethack_*${NACL_EXEEXT} \
      -s . \
      -o nethack.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py nethack.nmf
  popd
  # Create a copy of nethack for debugging.
  LogExecute cp ${ASSEMBLY_DIR}/nethack.nmf ${ASSEMBLY_DIR}/nethack_debug.nmf
  sed 's/nethack\.js/nethack_debug.js/' \
      ${ASSEMBLY_DIR}/nethack.html > ${ASSEMBLY_DIR}/nethack_debug.html
  sed 's/nethack\.nmf/nethack_debug.nmf/' \
      ${ASSEMBLY_DIR}/nethack.js > ${ASSEMBLY_DIR}/nethack_debug.js

  # Uncomment these lines to copy over source tree as a gdb sample.
  # Do not submit otherwise nethack source tree will be uploaded to
  # along with the rest of the 'out/publish' tree.
  #local ASSEMBLY_SRC_DIR="${PUBLISH_DIR}/nethack_src"
  #LogExecute rm -rf ${ASSEMBLY_SRC_DIR}
  #LogExecute cp -r ${SRC_DIR} ${ASSEMBLY_SRC_DIR}

  local MANIFEST_PATH="${PUBLISH_DIR}/nethack.manifest"
  LogExecute rm -f ${MANIFEST_PATH}
  pushd ${PUBLISH_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/genhttpfs.py \
      -r -o /tmp/nethack_manifest.tmp .
  LogExecute cp /tmp/nethack_manifest.tmp ${MANIFEST_PATH}
  popd

  InstallNaClTerm ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/manifest.json ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_16.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_48.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_128.png ${ASSEMBLY_DIR}
  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip nethack-${VERSION}.zip nethack
}
