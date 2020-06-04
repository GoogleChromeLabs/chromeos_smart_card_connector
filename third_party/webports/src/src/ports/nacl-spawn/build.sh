# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_CPPFLAGS+=" -I${START_DIR}"
EXECUTABLES="test/unittests"

if [ "${NACL_LIBC}" = "glibc" ]; then
  EXECUTABLES+=" test/elf_reader test/library_dependencies"
fi

if [ "${NACL_SHARED}" = "1" ]; then
  NACLPORTS_CFLAGS+=" -fPIC"
  NACLPORTS_CXXFLAGS+=" -fPIC"
  EXECUTABLES+=
fi

EnableGlibcCompat

ConfigureStep() {
  MakeDir ${BUILD_DIR}
  cp -rf ${START_DIR}/* ${BUILD_DIR}
}

BuildStep() {
  SetupCrossEnvironment
  export TOOLCHAIN
  export NACL_SHARED
  DefaultBuildStep
}

TestStep() {
  if [ "${NACL_LIBC}" = "glibc" ]; then
    SetupCrossEnvironment
    export TOOLCHAIN
    export NACL_SHARED
    LogExecute make test
  fi
  if [ "${TOOLCHAIN}" = "pnacl" ]; then
    RunSelLdrCommand test/unittests
  else
    LogExecute test/unittests.sh
  fi
}

InstallStep() {
  MakeDir ${DESTDIR_LIB}
  LogExecute cp libnacl_spawn.a ${DESTDIR_LIB}
  if [ "${NACL_SHARED}" = "1" ]; then
    LogExecute cp libnacl_spawn.so ${DESTDIR_LIB}
  fi
  LogExecute cp libcli_main.a ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}
  LogExecute cp -f ${START_DIR}/include/spawn.h ${DESTDIR_INCLUDE}/
  LogExecute cp -f ${START_DIR}/include/nacl_main.h ${DESTDIR_INCLUDE}/
  if [ "${TOOLCHAIN}" = "bionic" ]; then
    LogExecute cp -f ${START_DIR}/include/bsd_spawn.h ${DESTDIR_INCLUDE}/
  fi
}
