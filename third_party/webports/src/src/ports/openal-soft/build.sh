# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=openal-info

# Our cpuid.h file for i686 contains asm that doesn't validate so for
# now we disable use of cpuid.h.
# TODO(sbc): re-enable once this get fixed:
# https://code.google.com/p/nativeclient/issues/detail?id=3836
if [ "${NACL_ARCH}" = "i686" ]; then
  EXTRA_CMAKE_ARGS+=" -DHAVE_CPUID_H=0"
fi

# Defaults to dynamic lib, but newlib can only link statically.
if [ "${NACL_SHARED}" != "1" ]; then
  EXTRA_CMAKE_ARGS+=" -DLIBTYPE=STATIC"
fi
