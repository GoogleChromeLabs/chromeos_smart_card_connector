# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

ConfigureStep() {
  ChangeDir ${SRC_DIR}
  FILES="
my_syslog.h
syslog.cpp
Makefile"
  for FILE in $FILES; do
    cp -f ${START_DIR}/${FILE} .
  done

  LogExecute make -j${OS_JOBS} clean
}

BuildStep() {
  # export the nacl tools
  # The checked-in Makefile has more configuration for this example.
  SetupCrossEnvironment
  export NACLPORTS_CPPFLAGS
  export NACLPORTS_CFLAGS
  export NACLPORTS_LDFLAGS
  LogExecute make -j${OS_JOBS} thttpd
}

InstallStep() {
  MakeDir ${PUBLISH_DIR}
  install ${START_DIR}/thttpd.html ${PUBLISH_DIR}
  install ${START_DIR}/nacl.js ${PUBLISH_DIR}
  install ${START_DIR}/json2min.js ${PUBLISH_DIR}
  cp ${BUILD_DIR}/thttpd ${BUILD_DIR}/thttpd_${NACL_ARCH}${NACL_EXEEXT}
  install ${BUILD_DIR}/thttpd_${NACL_ARCH}${NACL_EXEEXT} ${PUBLISH_DIR}/
  ChangeDir ${PUBLISH_DIR}

  CMD="$NACL_SDK_ROOT/tools/create_nmf.py \
      -o thttpd.nmf -s . \
      thttpd_*${NACL_EXEEXT}"

  LogExecute $CMD
}
