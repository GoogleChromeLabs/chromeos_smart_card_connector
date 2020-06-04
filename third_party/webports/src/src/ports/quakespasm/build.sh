# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}/Quake
EXECUTABLES=quakespasm${NACL_EXEEXT}
MAKE_TARGETS="MP3LIB=mpg123"

ConfigureStep() {
  return
}

BuildStep() {
  make clean
  if [ "${NACL_DEBUG}" = "1" ]; then
    MAKE_TARGETS+=" DEBUG=1"
  fi
  SetupCrossEnvironment
  DefaultBuildStep
}

InstallStep() {
  MakeDir ${PUBLISH_DIR}
  LogExecute cp ${START_DIR}/index.html ${PUBLISH_DIR}
  LogExecute cp ${START_DIR}/quakespasm.js ${PUBLISH_DIR}
  LogExecute cp ${NACL_SRC}/third_party/zip.js/WebContent/*.js ${PUBLISH_DIR}
  LogExecute cp ${BUILD_DIR}/quakespasm${NACL_EXEEXT} \
      ${PUBLISH_DIR}/quakespasm_${NACL_ARCH}${NACL_EXEEXT}
  ChangeDir ${PUBLISH_DIR}
  LogExecute ${NACL_SDK_ROOT}/tools/create_nmf.py -s . \
      quakespasm*${NACL_EXEEXT} -o quakespasm.nmf
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    sed -i.bak 's/x-nacl/x-pnacl/' quakespasm.js
  fi
}
