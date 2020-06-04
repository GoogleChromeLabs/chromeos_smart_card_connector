# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=bin/makeconv
NACL_CONFIGURE_PATH=${SRC_DIR}/source/configure

# Disable packaging of data files. This doesn't seem to be well
# supported when cross compiling.
EXTRA_CONFIGURE_ARGS+=" --disable-extras"
EXTRA_CONFIGURE_ARGS+=" --with-data-packaging=archive"

if [ "${NACL_SHARED}" != "1" ]; then
  EXTRA_CONFIGURE_ARGS+=" --disable-shared --enable-static --disable-dyload"
fi

if [ "${NACL_LIBC}" = "newlib" ]; then
  # The newlib headers don't define tzset when --std=c++0x, only with
  # gnu++0x.  This prevents ICU from using its default of c++0x.
  NACLPORTS_CXXFLAGS+=" -std=gnu++0x"
fi

if [ "${TOOLCHAIN}" = "pnacl" ]; then
  # Samples don't link with the PNaCl toolchain becuase they try to
  # link agaist the non-stub version of libicudata.a, which we can't
  # use because its a native library produced by the pkgdata utility
  # which is not fully setup for cross compiling :(
  EXTRA_CONFIGURE_ARGS+=" --disable-samples"
else
  EXECUTABLES+=" samples/date/icudate"
fi

BuildHostICU() {
  Banner "Configuring host version of ${NAME}"
  if [ "${NACL_ARCH}" = "x86_64" ]; then
    HOST_BUILD_DIR=${WORK_DIR}/build_host_64
    export CPPFLAGS=-m64
    export LDFLAGS=-m64
  else
    HOST_BUILD_DIR=${WORK_DIR}/build_host_32
    export CPPFLAGS=-m32
    export LDFLAGS=-m32
  fi
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LogExecute ${NACL_CONFIGURE_PATH}
  Banner "Building host version of ${NAME}"
  LogExecute make
}

ConfigureStep() {
  BuildHostICU
  Banner "Configuring cross build of ${NAME}"
  NACL_ARFLAGS=
  EXTRA_CONFIGURE_ARGS+=" --with-cross-build=${HOST_BUILD_DIR}"
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}
