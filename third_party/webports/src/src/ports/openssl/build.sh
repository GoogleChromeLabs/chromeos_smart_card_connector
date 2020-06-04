# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# The openssl build can fail when build with -jN.
# TODO(sbc): Remove this if/when openssl is upgraded to a version that supports
# parallel make.
OS_JOBS=1
BUILD_DIR=${SRC_DIR}
INSTALL_TARGETS="install_sw INSTALL_PREFIX=${DESTDIR}"

EnableGlibcCompat

ConfigureStep() {
  if [ "${NACL_SHARED}" = "1" ] ; then
    local EXTRA_ARGS="shared"
  else
    local EXTRA_ARGS="no-dso"
  fi

  # Workaround for arm-gcc bug:
  # https://code.google.com/p/nativeclient/issues/detail?id=3205
  # TODO(sbc): remove this once the issue is fixed
  if [ "${NACL_ARCH}" = "arm" ]; then
    EXTRA_ARGS+=" -mfpu=vfp"
  fi
  EXTRA_ARGS+=" ${NACLPORTS_CPPFLAGS}"

  # Override $SYSTEM $RELEASE and $MACHINE, otherwise openssl's
  # config will use uname to try and guess them which has
  # different results depending on the host OS.
  local machine="le32${NACL_LIBC}"
  SYSTEM=nacl RELEASE=0 MACHINE=${machine} \
    CC=${NACLCC} AR=${NACLAR} RANLIB=${NACLRANLIB} \
    LogExecute ./config \
    --prefix=${PREFIX} no-asm no-hw no-krb5 ${EXTRA_ARGS} -D_GNU_SOURCE
}


BuildStep() {
  LogExecute make clean
  Banner "Building openssl"
  DefaultBuildStep
}


InstallStep() {
  DefaultInstallStep
  # openssl (for some reason) installs shared libraries with 555 (i.e.
  # not writable.  This causes issues when create_nmf copies the libraries
  # and then tries to overwrite them later.
  if [ "${NACL_SHARED}" = "1" ] ; then
    LogExecute chmod 644 ${DESTDIR_LIB}/lib*.so*
    LogExecute chmod 644 ${DESTDIR_LIB}/engines/lib*.so*
  fi
}


TestStep() {
  # TODO(jvoung): Integrate better with "make test".
  # We'd need to make util/shlib_wrap.sh run the sel_ldr scripts instead
  # of trying to run the nexes directly.
  local all_tests="bntest ectest ecdsatest ecdhtest exptest \
ideatest shatest sha1test sha256t sha512t mdc2test rmdtest md4test \
md5test hmactest wp_test rc2test rc4test bftest casttest \
destest randtest dhtest dsatest rsa_test enginetest igetest \
srptest asn1test md2test rc5test ssltest jpaketest"
  ChangeDir test
  LogExecute make
  export SEL_LDR_LIB_PATH=$PWD/..
  for test_name in ${all_tests}; do
    RunSelLdrCommand ${test_name}
  done
  RunSelLdrCommand evp_test evptests.txt
}
