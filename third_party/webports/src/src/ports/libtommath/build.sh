# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
MAKE_TARGETS=mtest
EXECUTABLES=mtest/mtest${NACL_EXEEXT}

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

BuildStep() {
  SetupCrossEnvironment
  DefaultBuildStep
}

TestStep() {
  # To run tests, pipe mtest.nexe output into test.nexe input
  #   mtest/mtest.exe | test.nexe
  # However, this test is setup to run forever, so we don't run
  # it as part of the build.
  #RunSelLdrCommand mtest/mtest.nexe | RunSelLdrCommand test.nexe
  return
}

InstallStep() {
  # copy libs and headers manually
  MakeDir ${DESTDIR_INCLUDE}
  MakeDir ${DESTDIR_LIB}
  LogExecute cp *.h ${DESTDIR_INCLUDE}/
  LogExecute cp *.a ${DESTDIR_LIB}/
}
