# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ "${NACL_SHARED}" = "0" ]; then
  EXTRA_CONFIGURE_ARGS+=" --enable-shared=no"
fi

EnableGlibcCompat

InstallStep() {
  DefaultInstallStep
  local pkgconfig_file=${INSTALL_DIR}/naclports-dummydir/lib/pkgconfig/x11.pc
  if [ "${NACL_LIBC}" = "newlib" ]; then
    if ! grep -Eq "lglibc-compat" $pkgconfig_file; then
      sed -i.bak 's/-lX11/-lX11 -lglibc-compat/' $pkgconfig_file
    fi
  fi
}

