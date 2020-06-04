# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

NACLPORTS_CPPFLAGS+=" -DHAVE_TERMIOS_H -DNO_CHMOD -DNO_FCHMOD -DNO_LCHMOD"
NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"

EXECUTABLES="funzip unzip unzipsfx"

BuildStep() {
  make -f unix/Makefile clean
  # "generic" target, which runs unix/configure, is
  # suggested. However, this target does not work well with NaCl. For
  # example, it does not allow us to overwrite LFLAGS1, it sets the
  # results of uname command, etc.
  #
  # We use NACLCXX as the linker because we link some C++ libraries
  # (e.g., libppapi).
  make -j${OS_JOBS} -f unix/Makefile unzips \
      CC=${NACLCC} LD=${NACLCXX} \
      CFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS}" LF2= \
      LFLAGS1="${NACLPORTS_LDFLAGS} ${NACL_CLI_MAIN_LIB}"
}

InstallStep() {
  LogExecute make -f unix/Makefile install prefix=${DESTDIR}/${PREFIX}
}

TestStep() {
  if [ "${TOOLCHAIN}" = "pnacl" ]; then
    return
  fi
  TESTZIP=testmake.zip
  LogExecute ./unzip.sh -bo ${TESTZIP} testmake.zipinfo
  CheckHash testmake.zipinfo f0d76aa12768455728f4bb0f42ceab64aaddfae8
}

PublishStep() {
  PublishMultiArch unzip unzip
}
