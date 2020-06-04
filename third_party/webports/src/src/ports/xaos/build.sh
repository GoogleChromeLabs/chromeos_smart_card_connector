# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=bin/xaos
NACL_CPPFLAGS+=" -D__NO_MATH_INLINES=1"

PatchStep() {
  DefaultPatchStep
  echo "copy nacl driver"
  cp -r "${START_DIR}/nacl-ui-driver" "${SRC_DIR}/src/ui/ui-drv/nacl"
}

ConfigureStep() {
  # xaos does not work with a build dir which is separate from the
  # src dir - so we copy src -> build
  ChangeDir ${SRC_DIR}
  Remove ${BUILD_DIR}
  local tmp=${SRC_DIR}.tmp
  Remove ${tmp}
  cp -r ${SRC_DIR} ${tmp}
  mv ${tmp} ${BUILD_DIR}

  ChangeDir ${BUILD_DIR}
  echo "running autoconf"
  LogExecute rm ./configure
  LogExecute autoconf

  # xaos takes case of defining NDEBUG itself
  NACLPORTS_CFLAGS="${NACLPORTS_CFLAGS/-DNDEBUG/}"
  NACLPORTS_LDFLAGS+=" -Wl,--undefined=PPP_GetInterface \
                  -Wl,--undefined=PPP_ShutdownModule \
                  -Wl,--undefined=PPP_InitializeModule \
                  -Wl,--undefined=original_main"

  export LIBS="-lppapi -lpthread -l${NACL_CXX_LIB} -lm"
  SetupCrossEnvironment
  LogExecute ./configure --with-png=no --with-long-double=no \
      --host=nacl --with-x11-driver=no --with-sffe=no
}

BuildStep() {
  SetupCrossEnvironment
  DefaultBuildStep
}

InstallStep(){
  MakeDir ${PUBLISH_DIR}
  install ${START_DIR}/xaos.html ${PUBLISH_DIR}
  install ${START_DIR}/xaos.nmf ${PUBLISH_DIR}
  # Not used yet
  install ${BUILD_DIR}/help/xaos.hlp ${PUBLISH_DIR}
  install ${BUILD_DIR}/bin/xaos ${PUBLISH_DIR}/xaos_${NACL_ARCH}${NACL_EXEEXT}
}
