# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

EnableGlibcCompat

if [ "${TOOLCHAIN}" = "pnacl" -o "${TOOLCHAIN}" = "clang-newlib" ]; then
  # TODO(sbc): Should probably use clang here
  COMPILER=gcc
else
  COMPILER=gcc
fi

if [ "${TOOLCHAIN}" = "pnacl" -o "${TOOLCHAIN}" = "clang-newlib" ]; then
  COMPILER_VERSION=3.6
elif [ "${NACL_ARCH}" = "arm" ]; then
  COMPILER_VERSION=4.8
else
  COMPILER_VERSION=4.4.3
fi

# TODO(eugenis): build dynamic libraries, too
BUILD_ARGS="
  toolset=${COMPILER}
  target-os=unix
  --build-dir=../${NACL_BUILD_SUBDIR}
  --stagedir=../${NACL_BUILD_SUBDIR}
  link=static"

BUILD_ARGS+=" --without-python"
BUILD_ARGS+=" --without-signals"
BUILD_ARGS+=" --without-mpi"
BUILD_ARGS+=" --without-context"
BUILD_ARGS+=" --without-coroutine"

if [ "${NACL_LIBC}" != "glibc" ] ; then
  BUILD_ARGS+=" --without-locale"
  BUILD_ARGS+=" --without-log"
  BUILD_ARGS+=" --without-test"
  BUILD_ARGS+=" --without-timer"

fi

if [ "${NACL_LIBC}" = "bionic" ] ; then
  # No statvfs on bionic
  BUILD_ARGS+=" --without-filesystem"
fi

if [ "${NACL_ARCH}" = "arm" -o "${NACL_ARCH}" = "pnacl" ]; then
  # The ARM toolchains are not currently built with full C++ threading support
  # (_GLIBCXX_HAS_GTHREADS is not defined)
  # PNaCl currently doesn't support -x c++-header which threading library
  # tries to use during the build.
  BUILD_ARGS+=" --without-math"
  BUILD_ARGS+=" --without-thread"
  BUILD_ARGS+=" --without-wave"
fi

ConfigureStep() {
  flags=
  for flag in ${NACLPORTS_CPPFLAGS}; do
    flags+=" <compileflags>${flag}"
  done
  conf="using ${COMPILER} : ${COMPILER_VERSION} : ${NACLCXX} :${flags} ;"
  echo $conf > tools/build/v2/user-config.jam
  LogExecute ./bootstrap.sh --prefix=${NACL_PREFIX}
}

BuildStep() {
  LogExecute ./b2 stage -j ${OS_JOBS} ${BUILD_ARGS}
}

InstallStep() {
  LogExecute ./b2 install -d0 --prefix=${DESTDIR}/${PREFIX} ${BUILD_ARGS}
}
