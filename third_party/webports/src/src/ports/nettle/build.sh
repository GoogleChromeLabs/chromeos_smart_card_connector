# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXTRA_CONFIGURE_ARGS+=" --disable-assembler"

if [ "${NACL_SHARED}" != "1" ]; then
  EXTRA_CONFIGURE_ARGS+=" --disable-shared"
fi

EXECUTABLES="
  ./tools/sexp-conv${NACL_EXEEXT}
  ./tools/pkcs1-conv${NACL_EXEEXT}
  ./tools/nettle-hash${NACL_EXEEXT}
  ./tools/nettle-lfib-stream${NACL_EXEEXT}"
