# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="src/protoc${NACL_EXEEXT}"
HOST_BUILD_DIR="${WORK_DIR}/build_host"
EXTRA_CONFIGURE_ARGS="--with-protoc=$HOST_BUILD_DIR/src/protoc"

if [ ${TOOLCHAIN} = "pnacl" ]; then
  # We have custom version of atomicops_internal_pnacl.h which uses
  # C++ atomics.  This is only needed for pnacl since __atomic_or_fetch
  # is currently not implemented:
  # https://code.google.com/p/nativeclient/issues/detail?id=3941
  # TODO(sbc): remove this once we fix the pnacl toolchain:
  NACLPORTS_CXXFLAGS+=" -std=gnu++11"
fi

BuildHostProtoc() {
  if [ ! -f "${HOST_BUILD_DIR}/src/protoc" ]; then
    MakeDir "${HOST_BUILD_DIR}"
    ChangeDir "${HOST_BUILD_DIR}"
    LogExecute "${SRC_DIR}/configure"
    # Work around bug with parallel builds by forcing pbconfig.h to be
    # built first.
    LogExecute make -C src google/protobuf/stubs/pbconfig.h
    LogExecute make -C src -j${OS_JOBS} protoc
    ChangeDir ${BUILD_DIR}
  fi
}

ConfigureStep() {
  BuildHostProtoc
  DefaultConfigureStep
}

TestStep() {
  # Still a few issues with the non-lite unittests
  # TODO(sbc): add protobuf-test${NACL_EXEEXT} to this list
  TESTS="protobuf-lite-test${NACL_EXEEXT} protobuf-test${NACL_EXEEXT}"
  ChangeDir gtest
  LogExecute make -j${OS_JOBS}
  ChangeDir ../src
  LogExecute make -j${OS_JOBS} ${TESTS}
  # The protobuf-test exectuable expects srcdir to be set so it
  # can find its source data.
  export srcdir=${SRC_DIR}/src
  for test in ${TESTS}; do
    RunSelLdrCommand ${test}
  done
}
