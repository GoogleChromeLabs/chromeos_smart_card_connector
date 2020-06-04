# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EXECUTABLES=quake${NACL_EXEEXT}

PatchStep() {
  DefaultPatchStep
  cp ${START_DIR}/Makefile ${BUILD_DIR}
}

ConfigureStep() {
  return
}

BuildStep() {
  make clean
  SetupCrossEnvironment
  DefaultBuildStep
}

InstallStep() {
  MakeDir ${PUBLISH_DIR}
  LogExecute cp ${START_DIR}/quake.html ${PUBLISH_DIR}
  LogExecute cp ${BUILD_DIR}/quake${NACL_EXEEXT} ${PUBLISH_DIR}
  ChangeDir ${PUBLISH_DIR}
  LogExecute ${NACL_SDK_ROOT}/tools/create_nmf.py -s . quake${NACL_EXEEXT} \
      -o quake.nmf
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    sed -i.bak 's/x-nacl/x-pnacl/' quake.html
  fi
}
