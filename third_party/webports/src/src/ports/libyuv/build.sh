# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="convert libyuv_unittest"

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

EnableGlibcCompat

if [ "${NACL_ARCH}" = "x86_64" ]; then
  NACLPORTS_CPPFLAGS+=" -DLIBYUV_DISABLE_X86=1"
fi

EXTRA_CMAKE_ARGS="-DTEST=ON"

TestStep() {
  # TODO(sbc): re-enable i686 testing once we fix this gtest-releated issue:
  # http://crbug.com/434821
  if [ "${NACL_ARCH}" = "i686" ]; then
    return
  fi
  if [ "${NACL_ARCH}" = pnacl ]; then
    return
  fi
  LogExecute ./libyuv_unittest.sh --gtest_filter=-libyuvTest.ARGBRect_Unaligned
}
