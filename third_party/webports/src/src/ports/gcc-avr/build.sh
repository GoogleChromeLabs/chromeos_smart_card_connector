# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

HOST_BUILD_DIR=${WORK_DIR}/build_host
HOST_BIN_INSTALL_DIR=${NACL_PACKAGES_BUILD}/binutils-2.25/install_host

AVR_LIBC_URL=http://storage.googleapis.com/naclports/mirror/avr-libc-1.8.1.tar.bz2
AVR_LIBC_SHA=b56fe21b30341869aa768689b0f6a07d896b17fa
AVR_LIBC_VERSION=avr-libc-1.8.1

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"
export PATH="${PATH}:${NACL_PACKAGES_BUILD}/binutils-2.25/install_host/bin"
NACLPORTS_CPPFLAGS+=" -Dpipe=nacl_spawn_pipe"

if [ "${NACL_LIBC}" = "newlib" ]; then
  export EXTRA_INCLUDES=" -I${NACLPORTS_INCLUDE}/glibc-compat"
fi

BuildHostGccAvr() {
  # Host avr-gcc already installed, skip install.
  if [ -e ${HOST_BIN_INSTALL_DIR}/bin/avr-gcc ]; then
    return
  fi
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  CC="gcc" EXTRA_LIBS="" LIBS="" \
    LogExecute ${SRC_DIR}/configure \
    --prefix=${HOST_BIN_INSTALL_DIR} \
    --target=avr \
    --enable-languages=c,c++ \
    --disable-nls --disable-libssp \
    --with-gmp=${NACL_PACKAGES_BUILD}/gmp/install_host \
    --with-mpfr=${NACL_PACKAGES_BUILD}/mpfr/install_host \
    --with-mpc=${NACL_PACKAGES_BUILD}/mpc/install_host
  EXTRA_INCLUDES="" EXTRA_LIBS="" LIBS="" LogExecute make
  EXTRA_INCLUDES="" EXTRA_LIBS="" LIBS="" LogExecute make install
}

EXTRA_CONFIGURE_ARGS="\
    --enable-languages=c,c++ --disable-nls \
    --disable-libssp --host=x86_64-nacl --target=avr"

if [ "${NACL_LIBC}" = "newlib" ]; then
  EXTRA_CONFIGURE_ARGS+=" --with-newlib"
fi

ConfigureStep() {
  for cache_file in $(find . -name config.cache); do
    Remove $cache_file
  done
  ChangeDir ${SRC_DIR}
  BuildHostGccAvr
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

AvrLibcDownload() {
  cd ${NACL_PACKAGES_CACHE}
  # If matching tarball already exists, don't download again
  if ! CheckHash ${AVR_LIBC_VERSION}.tar.bz2 ${AVR_LIBC_SHA}; then
    Fetch ${AVR_LIBC_URL} ${AVR_LIBC_VERSION}.tar.bz2
    if ! CheckHash ${AVR_LIBC_VERSION}.tar.bz2 ${AVR_LIBC_SHA} ; then
       Banner "${AVR_LIBC_VERSION}.tar.bz2 failed checksum!"
       exit -1
    fi
  fi
}

AvrLibcExtractAndInstall() {
  # Return is avr-libc is already installed.
  if [ -d ${WORK_DIR}/avr_libc_install ]; then
    cp -r ${WORK_DIR}/avr_libc_install/* ${INSTALL_DIR}/naclports-dummydir
    return
  fi

  # Install avr-libc
  Banner "Untarring ${AVR_LIBC_VERSION}"
  tar jxf ${NACL_PACKAGES_CACHE}/${AVR_LIBC_VERSION}.tar.bz2
  ChangeDir ${AVR_LIBC_VERSION}
  CC="avr-gcc"
    LogExecute ./configure \
    --prefix=${WORK_DIR}/avr_libc_install \
    --host=avr \
    --with-http=no \
    --with-html=no \
    --with-ftp=no \
    --with-x=no
  LogExecute make
  LogExecute make install

  # avr-libc needs to be in the same directory as the gcc-avr install
  cp -r ${WORK_DIR}/avr_libc_install/* ${INSTALL_DIR}/naclports-dummydir
}

InstallStep() {
  DefaultInstallStep
  AvrLibcDownload
  AvrLibcExtractAndInstall
}
