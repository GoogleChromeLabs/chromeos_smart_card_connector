# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The NaCL SDK includes a version of jsoncpp conflicts with this one.
# Remove the SDK include path so that these headers don't get used
# during the build.
NACLPORTS_CPPFLAGS=${NACLPORTS_CPPFLAGS/-I${NACL_SDK_ROOT}\/include/}

EXECUTABLES=src/test_lib_json/jsoncpp_test

EXTRA_CMAKE_ARGS="-DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF"

TestStep() {
  if [ "${TOOLCHAIN}" = "pnacl" ]; then
    return
  fi
  LogExecute src/test_lib_json/jsoncpp_test.sh
}
