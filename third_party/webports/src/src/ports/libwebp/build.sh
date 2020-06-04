# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ "${NACL_SHARED}" != "1" ]; then
  # Without this the test for libpng fails with undefined math functions
  NACLPORTS_LIBS="-lz -lm"
fi

EXECUTABLES="
  examples/dwebp${NACL_EXEEXT}
  examples/cwebp${NACL_EXEEXT}
"
