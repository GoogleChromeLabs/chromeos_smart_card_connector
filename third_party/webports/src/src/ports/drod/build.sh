# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=custom/bin/drod
BUILD_DIR=${SRC_DIR}/Caravel/Master/Linux
MAKE_TARGETS="drod-custom"
OS_JOBS=1

BuildStep() {
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  export AR="${NACLAR} cr"
  export RANLIB=${NACLRANLIB}
  export CXXFLAGS="${NACLPORTS_CXXFLAGS} -Wno-write-strings"
  export LDFLAGS_common=${NACLPORTS_LDFLAGS}
  export EXTRA_LIBS="-lSDL_mixer -lSDL -lmikmod -lvorbisfile -lvorbisenc \
                     -lvorbis -logg -lfreetype -lpng -lbz2 -lSDLmain -ltar \
                     -lnacl_io"
  # So sdl-config can be found
  export PATH=${NACLPORTS_BIN}:${PATH}
  DefaultBuildStep
}

InstallStep() {
  return
}

PublishStep() {
  # TODO(sbc): avoid duplicating the install step here.
  DESTDIR=${PUBLISH_DIR}/.data
  DefaultInstallStep

  ChangeDir ${DESTDIR}
  mv usr/local/games/drod ${PUBLISH_DIR}/drod_${NACL_ARCH}${NACL_EXEEXT}
  tar cf ${PUBLISH_DIR}/drod_usr.tar usr
  tar cf ${PUBLISH_DIR}/drod_var.tar var
  rm -rf ${DESTDIR}
  cp ${START_DIR}/drod.html ${PUBLISH_DIR}
  ChangeDir ${PUBLISH_DIR}
  python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      drod_*${NACL_EXEEXT} \
      -s . \
      -o drod.nmf
  if [ ${NACL_ARCH} = pnacl ]; then
    sed -i.bak 's/x-nacl/x-pnacl/g' ${PUBLISH_DIR}/drod.html
  fi
}
