# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

EXECUTABLES=xmltest

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

ConfigureStep() {
  LogExecute make clean
}

BuildStep() {
  SetupCrossEnvironment
  CFLAGS="${CPPFLAGS} ${CFLAGS}"
  CXXFLAGS="${CPPFLAGS} ${CXXFLAGS}"
  echo $CFLAGS
  LogExecute make libtinyxml.a xmltest
}

TestStep() {
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    return
  fi

  LogExecute ./xmltest.sh
}

InstallStep() {
  # copy libs and headers manually
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}/${PACKAGE_NAME}
  LogExecute cp *.h ${DESTDIR_INCLUDE}/${PACKAGE_NAME}/
  LogExecute cp *.a ${DESTDIR_LIB}/
}
