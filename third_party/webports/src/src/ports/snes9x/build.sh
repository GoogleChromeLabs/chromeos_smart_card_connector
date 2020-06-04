# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}/unix

EXECUTABLES=snes9x
NACL_CONFIGURE_PATH=${SRC_DIR}/unix/configure
EXTRA_CONFIGURE_ARGS="\
      --disable-gamepad \
      --disable-gzip \
      --disable-zip \
      --disable-jma \
      --disable-screenshot \
      --disable-netplay \
      --without-x \
      --enable-sound"
export EXTRA_LIBS="${NACLPORTS_LDFLAGS}"
EXTRA_LIBS+=" -lppapi_simple_cpp -lnacl_io -lppapi_cpp -lppapi"

AutoconfStep() {
  echo "Autoconf..."
  autoconf
  PatchConfigure
  PatchConfigSub
}

ConfigureStep() {
  AutoconfStep
  DefaultConfigureStep
}

InstallStep(){
  MakeDir ${PUBLISH_DIR}
  install ${START_DIR}/snes9x.html ${PUBLISH_DIR}
  install ${START_DIR}/snes9x.js ${PUBLISH_DIR}
  install ${BUILD_DIR}/snes9x ${PUBLISH_DIR}/snes9x_${NACL_ARCH}${NACL_EXEEXT}

  python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${PUBLISH_DIR}/snes9x_*${NACL_EXEEXT} \
      -s ${PUBLISH_DIR} \
      -o ${PUBLISH_DIR}/snes9x.nmf

  if [ "${NACL_ARCH}" = "pnacl" ]; then
    sed -i.bak 's/x-nacl/x-pnacl/' ${PUBLISH_DIR}/snes9x.js
  fi
}
