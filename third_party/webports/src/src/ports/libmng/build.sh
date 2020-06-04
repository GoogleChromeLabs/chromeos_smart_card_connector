# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

ConfigureStep() {
  # export the nacl tools
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  export AR=${NACLAR}
  export RANLIB=${NACLRANLIB}
  export CFLAGS="${NACLPORTS_CFLAGS} ${NACLPORTS_CPPFLAGS}"
  export LDFLAGS="${NACLPORTS_LDFLAGS}"
  ./unmaintained/autogen.sh --host=nacl --prefix=${PREFIX}
}
