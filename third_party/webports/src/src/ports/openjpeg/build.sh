# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="bin/opj_decompress bin/opj_compress"

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

if [ "${NACL_SHARED}" != "1" ]; then
  EXTRA_CMAKE_ARGS+=" -DBUILD_SHARED_LIBS=OFF"
fi

if [ "${NACL_ARCH}" = "pnacl" ]; then
  # The cmake endian test fails on PNaCl as it tries to search
  # for string constants in object files which doesn't work in
  # bitcode files. Instead disable the endian test and just use
  # the default (littleendian).
  EXTRA_CMAKE_ARGS+=" -DHAVE_OPJ_BIG_ENDIAN=TRUE"
fi
