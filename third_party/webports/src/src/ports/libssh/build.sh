# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EnableGlibcCompat

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

EXTRA_CMAKE_ARGS="\
  -DPNACL=ON\
  -DWITH_STATIC_LIB=ON\
  -DWITH_SHARED_LIB=OFF\
  -DWITH_EXAMPLES=OFF\
  -DHAVE_GETADDRINFO=ON
"
