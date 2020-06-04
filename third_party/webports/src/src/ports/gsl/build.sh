# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="
  gsl-randist${NACL_EXEEXT}
  gsl-histogram${NACL_EXEEXT}
  sys/test${NACL_EXEEXT}
"

ConfigureStep() {
  # TODO(sbc): Remove when this is fixed.
  # https://code.google.com/p/nativeclient/issues/detail?id=3205
  if [ "$NACL_ARCH" = "arm" ]; then
    NACLPORTS_CFLAGS="${NACLPORTS_CFLAGS/-O2/}"
  fi

  DefaultConfigureStep
}

BuildStep() {
  DefaultBuildStep
  LogExecute make -C sys test${NACL_EXEEXT}
}

TestStep() {
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    return
  fi
  LogExecute ./sys/test
}
