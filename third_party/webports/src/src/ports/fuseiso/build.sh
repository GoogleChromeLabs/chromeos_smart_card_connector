# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
NACLPORTS_CFLAGS+=" -I${NACL_SDK_ROOT}/include \
  -I${NACLPORTS_INCLUDE}/glib-2.0 \
  -I${NACL_PREFIX}/lib/glib-2.0/include \
  -I${NACLPORTS_INCLUDE}"

if [ "${NACL_LIBC}" = "newlib" ]; then
  NACLPORTS_CFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat"
fi

ConfigureStep() {
  LogExecute rm -f configure
  LogExecute autoconf
  DefaultConfigureStep
}

InstallStep() {
  # copy header manually
  DefaultInstallStep
  MakeDir ${DESTDIR_INCLUDE}
  LogExecute cp ${START_DIR}/fuseiso_lib.h ${DESTDIR_INCLUDE}/
}