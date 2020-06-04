# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ "${NACL_SHARED}" = "1" ]; then
  EXECUTABLES+=" libfreeimage-3.17.0.so"
fi

if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

BUILD_DIR=${SRC_DIR}

ConfigureStep() {
  return
}

BuildStep() {
  # export the nacl tools
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  export AR=${NACLAR}
  export RANLIB=${NACLRANLIB}
  export CFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS}"
  export CXXFLAGS="${NACLPORTS_CPPFLAGS} ${NACLPORTS_CXXFLAGS}"
  export PATH=${NACL_BIN_PATH}:${PATH}
  CFLAGS="${CFLAGS/-O2/-O1}"
  CXXFLAGS="${CXXFLAGS/-O2/-O1}"

  # assumes pwd has makefile
  LogExecute make -f Makefile.gnu clean
  LogExecute make -f Makefile.gnu -j${OS_JOBS} ENABLE_SHARED=${NACL_SHARED}
}

InstallStep() {
  export INCDIR=${DESTDIR_INCLUDE}
  export INSTALLDIR=${DESTDIR_LIB}
  LogExecute make -f Makefile.gnu install ENABLE_SHARED=${NACL_SHARED}
}
