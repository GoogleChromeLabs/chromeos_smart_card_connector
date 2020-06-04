# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=test/yajl_test
EXTRA_CMAKE_ARGS="-DBUILD_SHARED=${NACL_SHARED}"

TestStep() {
  local script=${BUILD_DIR}/test/yajl_test.sh
  if [ ${NACL_ARCH} = "pnacl" ]; then
    local pexe=test/yajl_test
    TranslateAndWriteLauncherScript ${pexe} x86-32 ${pexe}.x86-32.nexe "${script}"
    (cd "${SRC_DIR}/test" && ./run_tests.sh "${script}")
    TranslateAndWriteLauncherScript ${pexe} x86-64 ${pexe}.x86-64.nexe "${script}"
    (cd "${SRC_DIR}/test" && ./run_tests.sh "${script}")
  else
    (cd "${SRC_DIR}/test" && ./run_tests.sh "${script}")
  fi
}

# Override configure step to force it use CMake.  Without
# this the default configure step will see the ruby ./configure
# script at the top level of yajl (which doesn't work) and
# try to run that.
ConfigureStep() {
  ConfigureStep_CMake
}
