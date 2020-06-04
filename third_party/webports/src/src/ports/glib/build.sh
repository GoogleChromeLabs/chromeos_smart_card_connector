# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="
  gio/gapplication${NACL_EXEEXT}
  glib/gtester${NACL_EXEEXT}
  gobject/gobject-query${NACL_EXEEXT}
"

EnableGlibcCompat

ConfigureStep() {
  NACL_BIT_ARCH_ID=64
  if [ "${NACL_ARCH}" = "i686" ]; then
      NACL_BIT_ARCH_ID=32
  fi

  if [ "${NACL_LIBC}" = "newlib" ]; then
    if ! [ -e "${NACLPORTS_LIBDIR}/libffi.a" ]; then
      ln ${NACLPORTS_LIBDIR}/../lib${NACL_BIT_ARCH_ID}/libffi.a \
          ${NACLPORTS_LIBDIR}/libffi.a
    fi
  fi

  # fix libffi paths to work with glib and other libraries
  if [ "${NACL_LIBC}" = "glibc" ]; then
    if ! [ -e "${NACLPORTS_LIBDIR}/libffi.so.6" ]; then
      ln ${NACLPORTS_LIBDIR}/../lib${NACL_BIT_ARCH_ID}/libffi.so.6.0.4 \
          ${NACLPORTS_LIBDIR}/libffi.so.6
    fi
    sed -i.bak 's/\/\//\//g' ${NACLPORTS_LIBDIR}/*.la
    if ! [ -e "${NACLPORTS_LIBDIR}/libffi.so" ]; then
      ln ${NACLPORTS_LIBDIR}/../lib${NACL_BIT_ARCH_ID}/libffi.so.6.0.4 \
          ${NACLPORTS_LIBDIR}/libffi.so
    fi
    if ! [ -e "${NACLPORTS_LIBDIR}/libffi.la" ]; then
      ln ${NACLPORTS_LIBDIR}/../lib${NACL_BIT_ARCH_ID}/libffi.la \
          ${NACLPORTS_LIBDIR}/libffi.la
    fi
  fi

  NACLPORTS_CPPFLAGS+=" -DNVALGRIND=1"

  SetupCrossEnvironment

  export glib_cv_stack_grows=no
  export glib_cv_uscore=no
  export ac_cv_func_issetugid=no
  export ac_cv_func_posix_getpwuid_r=yes
  export ac_cv_func_posix_getgrgid_r=yes

  LogExecute ${SRC_DIR}/configure \
    --host=nacl \
    --prefix=${PREFIX} \
    --disable-libelf \
    --${NACL_OPTION}-mmx \
    --${NACL_OPTION}-sse \
    --${NACL_OPTION}-sse2 \
    --${NACL_OPTION}-asm
}

InstallStep(){
  # fix pkgconfig files to explicitly include libffi
  # for things that depend on glib
  sed -i 's/-lglib-2.0 -lintl/-lglib-2.0 -lffi -lintl/'\
   ${BUILD_DIR}/glib-2.0.pc

  DefaultInstallStep
}
