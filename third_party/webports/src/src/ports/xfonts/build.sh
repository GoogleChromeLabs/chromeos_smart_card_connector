# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  return
}

InstallStep() {
  MakeDir ${PUBLISH_DIR}
  ChangeDir ${PUBLISH_DIR}

  LogExecute rm -rf share
  MakeDir share/fonts
  LogExecute cp -fR ${NACL_PREFIX}/share/fonts ${PUBLISH_DIR}/share/
  for dir in $(find ${PUBLISH_DIR}/share/fonts/X11 -type d); do
    LogExecute mkfontscale "$dir"
    LogExecute mkfontdir "$dir"
  done

  LogExecute tar cf xorg-fonts.tar share/
  LogExecute shasum xorg-fonts.tar > xorg-fonts.tar.hash

  # Generate Fontconfig files
  if [ ${TOOLCHAIN} != "pnacl" ]; then
    local CACHE_OUT_DIR="${PUBLISH_DIR}/${NACL_ARCH}"
    MakeDir ${CACHE_OUT_DIR}/naclports-dummydir/var/cache/fontconfig/
    ChangeDir ${CACHE_OUT_DIR}
    LogExecute ${NACL_SDK_ROOT}/tools/sel_ldr.py --\
     ${NACL_PREFIX}/bin/fc-cache${NACL_EXEEXT} -sfv ./share/fonts\
      -y ${CACHE_OUT_DIR}
    LogExecute tar -cf xorg-fonts-cache-${NACL_ARCH}.tar naclports-dummydir/
    LogExecute shasum xorg-fonts-cache-${NACL_ARCH}.tar >\
     xorg-fonts-cache-${NACL_ARCH}.tar.hash
    LogExecute rm -rf naclports-dummydir
  fi

  LogExecute rm -rf share
}
