# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  # TODO: Remove when this is fixed.
  # https://code.google.com/p/nativeclient/issues/detail?id=3205
  if [ "$NACL_ARCH" = "arm" ]; then
    export NACLPORTS_CFLAGS="${NACLPORTS_CFLAGS/-O2/-O1}"
  fi
  DefaultConfigureStep
}
