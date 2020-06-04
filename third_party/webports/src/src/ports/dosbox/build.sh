# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=src/dosbox${NACL_EXEEXT}
MAKE_TARGETS="AR=${NACLAR}"

ConfigureStep() {
  SetupCrossEnvironment

  local conf_host=${NACL_CROSS_PREFIX}
  if [ ${NACL_ARCH} = "pnacl" ]; then
    # The PNaCl tools use "pnacl-" as the prefix, but config.sub
    # does not know about "pnacl".  It only knows about "le32-nacl".
    # Unfortunately, most of the config.subs here are so old that
    # it doesn't know about that "le32" either.  So we just say "nacl".
    conf_host="nacl"
  fi

  CONFIG_FLAGS="--host=${conf_host} \
      --prefix=${PREFIX} \
      --with-sdl-prefix=${NACL_TOOLCHAIN_ROOT} \
      --disable-shared \
      --with-sdl-exec-prefix=${NACL_TOOLCHAIN_ROOT}"

  # TODO(clchiou): Sadly we cannot export LIBS and LDFLAGS to configure, which
  # would fail due to multiple definitions of main and missing pp::CreateModule.
  # So we patch auto-generated Makefile after running configure.
  export PPAPI_LIBS=""
  export LIBS="-lnacl_io"
  LogExecute ${SRC_DIR}/configure ${CONFIG_FLAGS}

  SED_PREPEND_LIBS="s|^LIBS = \(.*$\)|LIBS = ${PPAPI_LIBS} \1|"
  SED_REPLACE_LDFLAGS="s|^LDFLAGS = .*$|LDFLAGS = ${NACLPORTS_LDFLAGS}|"

  find . -name Makefile -exec cp {} {}.bak \; \
      -exec sed -i.bak "${SED_PREPEND_LIBS}" {} \; \
      -exec sed -i.bak "${SED_REPLACE_LDFLAGS}" {} \;
}

InstallStep(){
  MakeDir ${PUBLISH_DIR}
  LogExecute install ${START_DIR}/dosbox.html ${PUBLISH_DIR}
  LogExecute install src/dosbox${NACL_EXEEXT} \
    ${PUBLISH_DIR}/dosbox_${NACL_ARCH}${NACL_EXEEXT}
  local CREATE_NMF="${NACL_SDK_ROOT}/tools/create_nmf.py"
  LogExecute ${CREATE_NMF} -s ${PUBLISH_DIR} ${PUBLISH_DIR}/dosbox_*${NACL_EXEEXT} -o ${PUBLISH_DIR}/dosbox.nmf

  if [ "${NACL_ARCH}" = "pnacl" ]; then
    sed -i.bak 's/x-nacl/x-pnacl/' ${PUBLISH_DIR}/dosbox.html
  fi
}
