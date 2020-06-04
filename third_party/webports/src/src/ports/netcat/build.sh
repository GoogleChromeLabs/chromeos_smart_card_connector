# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EnableGlibcCompat

BuildStep() {
  # netcat's configure script doesn't check for CXX but we patch
  # the Makefile to use it, so we need to it be defined at make
  # time
  export CXX=${NACLCXX}
  DefaultBuildStep
}
