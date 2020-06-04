# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


BuildStep() {
  ChangeDir ${SRC_DIR}
  local cflags="${NACLPORTS_CFLAGS} -I${NACLPORTS_INCLUDE}/freetype2"
  if [ ${NACL_ARCH} != "pnacl" ]; then
    cflags+=" -O3 -fomit-frame-pointer"
  fi
  if [ ${NACL_ARCH} = "i686" -o ${NACL_ARCH} = "x86_64" ]; then
    cflags+=" -mfpmath=sse -msse"
  fi
  # Workaround for arm-gcc bug:
  # https://code.google.com/p/nativeclient/issues/detail?id=3205
  # TODO(sbc): remove this once the issue is fixed
  if [ "${NACL_ARCH}" = "arm" ]; then
    cflags+=" -mfpu=vfp"
  fi
  MAKEFLAGS="-j${OS_JOBS}" AGGCXXFLAGS="${cflags}" LogExecute make -j${OS_JOBS}
}


InstallStep() {
  # copy libs and headers manually
  MakeDir ${DESTDIR_LIB}
  INCDIR=${DESTDIR_INCLUDE}/${NAME}-${VERSION}
  MakeDir ${INCDIR}
  LogExecute cp -R ${SRC_DIR}/include/*.h ${INCDIR}/
  LogExecute cp ${SRC_DIR}/font_freetype/*.h ${INCDIR}/
  LogExecute cp ${SRC_DIR}/src/libagg.a ${DESTDIR_LIB}
  LogExecute cp ${SRC_DIR}/font_freetype/libaggfontfreetype.a ${DESTDIR_LIB}/
}
