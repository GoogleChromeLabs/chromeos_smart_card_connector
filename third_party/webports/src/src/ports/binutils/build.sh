# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export ac_cv_func_getrlimit=no

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"
EXTRA_CONFIGURE_ARGS="\
  --enable-targets=x86_64-nacl,arm-nacl,avr \
  --disable-werror \
  --enable-deterministic-archives \
  --without-zlib"

BuildStep() {
  export CONFIG_SITE
  DefaultBuildStep
}

InstallStep() {
  DefaultInstallStep

  # The ldscripts that ship with this verion of binutils doesn't seem to
  # work with the glibc toolchain devenv.  Instead we copy the linker scripts
  # out of the SDK root.
  rm -rf ${DESTDIR}${PREFIX}/${NACL_ARCH}-nacl/lib/ldscripts
  LogExecute cp -r \
   ${NACL_SDK_ROOT}/toolchain/${OS_SUBDIR}_x86_glibc/x86_64-nacl/lib/ldscripts \
   ${DESTDIR}${PREFIX}/${NACL_ARCH}-nacl/lib/
}
