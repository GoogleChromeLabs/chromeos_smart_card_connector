# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

BuildStep() {
  export EXTRA_CFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS}"
  export LDFLAGS="${NACLPORTS_LDFLAGS} ${NACL_CLI_MAIN_LIB}"
  if [ "$NACL_LIBC" = "glibc" ]; then
    LDFLAGS+=" -ldl"
  fi
  CC=${NACLCC} LogExecute make clean
  CC=${NACLCC} LogExecute make nacl
}


InstallStep() {
  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR="${PUBLISH_DIR}/mongoose"
  MakeDir ${ASSEMBLY_DIR}
  LogExecute cp mongoose \
      ${ASSEMBLY_DIR}/mongoose_${NACL_ARCH}${NACL_EXEEXT}
  LogExecute cp ${START_DIR}/index.html ${ASSEMBLY_DIR}
  pushd ${ASSEMBLY_DIR}
  python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      mongoose_*${NACL_EXEEXT} \
      -s . \
      -o mongoose.nmf
  popd

  InstallNaClTerm ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/background.js ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/mongoose.js ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/manifest.json ${ASSEMBLY_DIR}
}
