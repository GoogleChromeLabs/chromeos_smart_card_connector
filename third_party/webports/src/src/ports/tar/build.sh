# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"
EXECUTABLES=src/tar${NACL_EXEEXT}

# The default when cross compiling is to assume chown does not
# follow symlinks, and the code that works around this in chown.c
# does not compile under newlib (missing O_NOCTTY).
export gl_cv_func_chown_follows_symlink=yes

# The mtio.h we currently ship with the arm/glibc toolchain is not usable
export ac_cv_header_sys_mtio_h=no

EnableGlibcCompat

if [ "${TOOLCHAIN}" = "glibc" -a "${NACL_ARCH}" != "arm" ]; then
  # TODO(sbc): remove this once we fix glibc issue:
  # https://code.google.com/p/nativeclient/issues/detail?id=4241
  NACLPORTS_CPPFLAGS+=" -D_FORTIFY_SOURCE=0"
fi

if [ "${NACL_ARCH}" = "arm" ]; then
  # The ARM compiler is a more recent version of gcc that generated a warnings
  # (which are errors due to Werror) unless we disable them.
  EXTRA_CONFIGURE_ARGS+=" --disable-gcc-warnings"
fi

if [ "${TOOLCHAIN}" = "pnacl" -o "${TOOLCHAIN}" = "clang-newlib" ]; then
  # correctly handle 'extern inline'
  NACLPORTS_CPPFLAGS+=" -std=gnu89"
fi

PublishStep() {
  MakeDir ${PUBLISH_DIR}
  cp src/tar${NACL_EXEEXT} ${PUBLISH_DIR}/tar_${NACL_ARCH}${NACL_EXEEXT}
  ChangeDir ${PUBLISH_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${PUBLISH_DIR}/tar_*${NACL_EXEEXT} \
      -s . \
      -o tar.nmf
}
