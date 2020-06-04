# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  return
}

InstallStep() {
  MakeDir ${INSTALL_DIR}${PREFIX}
  ChangeDir ${INSTALL_DIR}${PREFIX}

  if [ "${NACL_SHARED}" = "1" ]; then
    MakeDir lib
    # Copy core libraries
    LogExecute cp -r ${NACL_SDK_LIB}/*.so* lib/

    # Copy SDK libs
    LogExecute cp -r ${NACL_SDK_LIBDIR}/lib*.so* lib/
    LogExecute rm -fr lib/libgtest.*so*
    LogExecute rm -fr lib/libgmock.*so*
  fi
}
