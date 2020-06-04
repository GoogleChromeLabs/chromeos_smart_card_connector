# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="ninja"

NACLPORTS_LIBS+=" ${NACL_CLI_MAIN_LIB}"
NACLPORTS_CPPFLAGS+=" -Dpipe=nacl_spawn_pipe"

EnableGlibcCompat

BuildHostNinja() {
  # Build a host version ninja in $SRC_DIR
  if [ -f "${SRC_DIR}/ninja" ];then
    return
  fi
  ChangeDir ${SRC_DIR}
  LogExecute python ./configure.py --bootstrap
  ChangeDir ${BUILD_DIR}
}

ConfigureStep() {
  BuildHostNinja
  SetupCrossEnvironment
  # ninja doesn't honor CPPFLAGS to add them to CFLAGS
  CFLAGS+=" ${CPPFLAGS}"
  LogExecute python ${SRC_DIR}/configure.py --host=linux --platform=nacl
}

BuildStep() {
  LogExecute ${SRC_DIR}/ninja
}

TestStep() {
  LogExecute ${SRC_DIR}/ninja ninja_test
  RunSelLdrCommand ninja_test --gtest_filter=-SubprocessTest.*
}

InstallStep() {
  MakeDir ${DESTDIR}${PREFIX}/bin
  LogExecute cp ninja ${DESTDIR}${PREFIX}/bin/
}
