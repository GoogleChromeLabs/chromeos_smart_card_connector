# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export LIBS="-lnacl_io"

EnableGlibcCompat

if [ "${NACL_ARCH}" = "pnacl" ]; then
   NACLPORTS_CPPFLAGS+=" -DBYTE_ORDER=LITTLE_ENDIAN"
fi

NACLPORTS_CPPFLAGS+=" -DZMQ_FORCE_MUTEXES"

ConfigureStep() {
  export CROSS_COMPILE=true
  EXTRA_CONFIGURE_ARGS+=" --disable-shared --enable-static --with-poller=poll"
  DefaultConfigureStep
}
