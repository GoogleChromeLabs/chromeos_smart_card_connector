# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CTEST_EXECUTABLES="
  basicstuff
  cholesky
  determinant
  geo_transformations
  inverse
"

EXECUTABLES="
  test/basicstuff
  test/cholesky
  test/determinant
  test/geo_transformations
  test/inverse
"

EXTRA_CMAKE_ARGS="-DEIGEN_BUILD_PKGCONFIG=OFF -DEIGEN_SPLIT_LARGE_TESTS=OFF"
MAKE_TARGETS="$CTEST_EXECUTABLES"

TestStep() {
  # Eigen has ~600 tests, we only build a few
  ChangeDir ${BUILD_DIR}
  if [ ${NACL_ARCH} = "pnacl" ]; then
    return
  fi
  for exe in ${CTEST_EXECUTABLES}; do
    LogExecute test/$exe.sh
  done
}