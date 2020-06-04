# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  pushd ${SRC_DIR}/test
  ./autogen.sh
  popd

  Banner "Configuring ${PACKAGE_NAME}"
  SetupCrossEnvironment

  local conf_host=${NACL_CROSS_PREFIX}
  if [ ${NACL_ARCH} = "pnacl" ]; then
    # The PNaCl tools use "pnacl-" as the prefix, but config.sub
    # does not know about "pnacl".  It only knows about "le32-nacl".
    # Unfortunately, most of the config.subs here are so old that
    # it doesn't know about that "le32" either.  So we just say "nacl".
    conf_host="nacl"
  fi

  LIBS="$LDFLAGS" LogExecute ${SRC_DIR}/test/configure \
    --host=${conf_host} --prefix=${PREFIX}
}

InstallStep() {
  Remove ${PUBLISH_DIR}
  MakeDir ${PUBLISH_DIR}
  LogExecute cp *${NACL_EXEEXT} ${PUBLISH_DIR}
  ChangeDir ${SRC_DIR}/test
  LogExecute cp *.bmp *.wav *.xbm *.dat *.txt ${PUBLISH_DIR}
  ChangeDir ${PUBLISH_DIR}
  for NEXE in *${NACL_EXEEXT}; do
    LogExecute "${NACL_SDK_ROOT}/tools/create_html.py" ${NEXE}
  done
  if [ ${NACL_ARCH} = "pnacl" ]; then
    sed -i.bak 's/x-nacl/x-pnacl/' *.html
  fi
}
