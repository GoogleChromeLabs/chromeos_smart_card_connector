# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# parallel build usually fails (at least for me)
OS_JOBS=1
MAKE_TARGETS="f2clib blaslib lib"

ConfigureStep() {
  if [ $OS_NAME != "Darwin" ]; then
    # -u: do not rewrite newer with older files
    UPDATE=-u
  else
    UPDATE=
  fi

  MakeDir ${BUILD_DIR}
  ChangeDir ${BUILD_DIR}
  LogExecute cp ${UPDATE} -R ${SRC_DIR}/BLAS ${SRC_DIR}/F2CLIBS \
    ${SRC_DIR}/INSTALL ${SRC_DIR}/TESTING ${SRC_DIR}/INCLUDE ${SRC_DIR}/SRC ./
  LogExecute cp ${UPDATE} ${SRC_DIR}/Makefile ${SRC_DIR}/make.inc ./

  # make does not create it, but build relays on it being there
  LogExecute install -d SRC/VARIANTS/LIB
}

BuildStep() {
  # Without the -m option the x86_64 and arm builds break with (x86_64)
  # at build-nacl-x86_64-glibc/F2CLIBS/f2clibs
  #
  # …/nacl_sdk/pepper_canary/toolchain/linux_x86_glibc/bin/x86_64-nacl-ld  -r
  # -o main.xxx main.o
  #
  # Relocatable linking with relocations from format elf64-x86-64-nacl (main.o)
  # to format elf32-x86-64-nacl (main.xxx) is not supported.
  # TODO: Investigate/remove this and a later date
  LDEMULATION=
  if [ "${NACL_ARCH}" == "x86_64" ]; then
    LDEMULATION=-melf_x86_64_nacl
  fi
  if [ "${NACL_ARCH}" == "arm" ]; then
    LDEMULATION=-marmelf_nacl
    # Workaround for arm-gcc bug:
    # https://code.google.com/p/nativeclient/issues/detail?id=3205
    # TODO(sbc): remove this once the issue is fixed
    export CFLAGS=-mfpu=vfp
  fi
  export LDEMULATION
  export BUILD_CC=cc
  DefaultBuildStep
}

TestStep() {
  ChangeDir ${BUILD_DIR}/INSTALL
  RunSelLdrCommand ./testlsame
  RunSelLdrCommand ./testslamch
  RunSelLdrCommand ./testdlamch
  RunSelLdrCommand ./testsecond
  RunSelLdrCommand ./testdsecnd
  RunSelLdrCommand ./testversion
}

# the Makefile does not contain install target
InstallStep() {
  MakeDir ${DESTDIR_LIB}
  LogExecute install libblas.a ${DESTDIR_LIB}/
  LogExecute install liblapack.a ${DESTDIR_LIB}/
  LogExecute install tmglib.a ${DESTDIR_LIB}/
  LogExecute install F2CLIBS/libf2c.a ${DESTDIR_LIB}/
}
