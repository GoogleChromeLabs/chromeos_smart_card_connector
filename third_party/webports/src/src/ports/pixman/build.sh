# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=test/pixel-test${NACL_EXEEXT}

EXTRA_CONFIGURE_ARGS="
  --disable-arm-simd
  --disable-arm-iwmmxt
  --disable-arm-iwmmxt2
  --disable-arm-neon"

export PERL=/bin/true
