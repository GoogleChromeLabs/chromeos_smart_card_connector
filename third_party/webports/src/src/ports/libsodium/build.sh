# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXTRA_CONFIGURE_ARGS="--disable-pie --disable-shared"

RunTest() {
  naclport_test/crypto_box_test
}

TestStep() {
  MakeDir naclport_test/lib

  # the libtool warns "libtool: install: warning: remember to run
  # `libtool --finish pepper_31/toolchain/linux_pnacl/usr/lib'"
  (cd src/libsodium;
  /bin/bash ../../libtool --mode=install /usr/bin/install \
    -c libsodium.la $(cd ../../naclport_test/lib && pwd))

  if [[ ${NACL_ARCH} == pnacl ]]; then
    EXT=.bc
  else
    EXT=${NACL_EXEEXT}
  fi

  # on newlib_arm compilation crashed when without -lssp,
  # on other platforms it was ok without it
  LSSP=""
  if [[ ${NACL_ARCH} == arm && ${TOOLCHAIN} == newlib ]]; then
    LSSP="-lssp"
  fi
  INCLUDES="-Isrc/libsodium/include -Isrc/libsodium/include/sodium  \
            -I${SRC_DIR}/src/libsodium/include \
            -I${SRC_DIR}/src/libsodium/include/sodium"
  ${NACLCXX} ${INCLUDES} ${NACLPORTS_CPPFLAGS} ${NACLPORTS_CFLAGS} \
    ${NACLPORTS_LDFLAGS} -o naclport_test/crypto_box_test${EXT} \
    ${START_DIR}/crypto_box_test.c naclport_test/lib/libsodium.a \
    -lnacl_io -lpthread ${LSSP}

  [[ ${NACL_ARCH} == "pnacl" ]] && ${PNACLFINALIZE} \
    -o naclport_test/crypto_box_test${NACL_EXEEXT} naclport_test/crypto_box_test${EXT}

  echo "Running test"

  if [ "${NACL_ARCH}" = "pnacl" ]; then
    local pexe=crypto_box_test${NACL_EXEEXT}
    (cd naclport_test;
     TranslateAndWriteLauncherScript ${pexe} x86-32 crypto_box_test.x86-32${EXT} crypto_box_test)
    RunTest
    (cd naclport_test;
     TranslateAndWriteLauncherScript ${pexe} x86-64 crypto_box_test.x86-64${EXT} crypto_box_test)
    RunTest
    echo "Tests OK"
  elif [ "$(uname -m)" = "${NACL_ARCH_ALT}" ]; then
    WriteLauncherScript naclport_test/crypto_box_test crypto_box_test${EXT}
    RunTest
    echo "Tests OK"
  fi
}
