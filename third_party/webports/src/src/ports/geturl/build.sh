# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  MakeDir ${BUILD_DIR}
  cp -rf ${START_DIR}/* ${BUILD_DIR}
}

BuildStep() {
  MAKEFLAGS+=" NACL_ARCH=${NACL_ARCH_ALT}"
  export CFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS}"
  export CXXFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CXXFLAGS}"
  export LDFLAGS="${NACLPORTS_LDFLAGS}"
  DefaultBuildStep
}

InstallStep() {
  PublishMultiArch geturl_${NACL_ARCH_ALT}${NACL_EXEEXT} geturl
}
