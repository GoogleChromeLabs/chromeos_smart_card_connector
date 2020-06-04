#!/bin/bash
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(mtomasz): Remove this package, once a new upstream release of libarchive
# is available.

AutogenStep() {
  ChangeDir ${SRC_DIR}
  export MAKE_LIBARCHIVE_RELEASE="1"
  ./build/autogen.sh
  PatchConfigure
  cd -
}

if [ "${NACL_SHARED}" = "0" ]; then
  EXTRA_CONFIGURE_ARGS+=" --enable-shared=no"
fi

EnableGlibcCompat

ConfigureStep() {
  AutogenStep

  EXTRA_CONFIGURE_ARGS="--disable-bsdtar --disable-bsdcpio"
  EXTRA_CONFIGURE_ARGS+=" --without-iconv"

  # Temporary xml2 support cannot be added because the patch used in
  # ports/libarchve doesn't apply correctly here due. The reason is that
  # configure file is not present on gihub repository and is created
  # after AutogenStep.
  # # TODO(cmihail): Remove this once nacl.patch is applied correctly.
  EXTRA_CONFIGURE_ARGS+=" --without-xml2"

  NACLPORTS_CPPFLAGS+=" -Dtimezone=_timezone"

  DefaultConfigureStep
}

BuildHost() {
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  HOST_INSTALL_DIR=${WORK_DIR}/install_host
  if [ ! -d ${HOST_INSTALL_DIR} ]; then
    Banner "Build host version"
    MakeDir ${HOST_BUILD_DIR}
    ChangeDir ${HOST_BUILD_DIR}
    LogExecute ${SRC_DIR}/configure --without-lzma
    LogExecute make -j${OS_JOBS}
    LogExecute make install DESTDIR=${HOST_INSTALL_DIR}
    cd -
  fi
}

BuildStep() {
  BuildHost
  DefaultBuildStep
}
