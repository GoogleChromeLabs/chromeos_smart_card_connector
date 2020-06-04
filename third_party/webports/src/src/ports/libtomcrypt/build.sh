# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

MAKE_TARGETS="-f makefile.unix test"
EXECUTABLES=test

BuildStep() {
  SetupCrossEnvironment
  DefaultBuildStep
}

TestStep() {
  RunSelLdrCommand test
}

InstallStep() {
  # copy libs and headers manually
  MakeDir ${DESTDIR_INCLUDE}
  MakeDir ${DESTDIR_LIB}
  LogExecute cp src/headers/*.h ${DESTDIR_INCLUDE}
  LogExecute cp *.a ${DESTDIR_LIB}
}
