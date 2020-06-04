# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Linux disk image
readonly LINUX_IMG_URL=http://storage.googleapis.com/nativeclient-mirror/nacl/bochs-linux-img.tar.gz
readonly LINUX_IMG_NAME=linux-img
readonly LINUX_IMG_SHA1=70131438bd90534fa045c737017a2ca9a437bf5d

BOCHS_EXAMPLE_DIR=${NACL_SRC}/ports/bochs
EXECUTABLES=bochs

ConfigureStep() {
  SetupCrossEnvironment

  EXE=${NACL_EXEEXT} LogExecute ${SRC_DIR}/configure \
    --host=nacl \
    --prefix=${PREFIX} \
    --with-x=no \
    --with-x11=no \
    --with-sdl=yes \
    --with-gnu-ld
}

# $1 - filename
# $2 - root directory of archive
ImageExtractStep() {
  Banner "Untaring $1 to $2"
  ChangeDir ${WORK_DIR}
  Remove $2
  if [ $OS_SUBDIR = "windows" ]; then
    tar --no-same-owner -zxf ${NACL_PACKAGES_CACHE}/$1
  else
    tar zxf ${NACL_PACKAGES_CACHE}/$1
  fi
}

BuildStep() {
  # boch's Makefile runs sdl-config so we need to cross envrionment setup
  # during build as well as configure.
  SetupCrossEnvironment
  DefaultBuildStep
}

InstallStep() {
  ChangeDir ${SRC_DIR}
  mkdir -p img/usr/local/share/bochs/
  cp -r ${WORK_DIR}/${LINUX_IMG_NAME} img/
  mv img/linux-img/bochsrc old-bochsrc
  cp ${START_DIR}/bochsrc img/linux-img/bochsrc
  cp -r ${SRC_DIR}/bios/VGABIOS-lgpl-latest img/
  cp -r ${SRC_DIR}/bios/BIOS-bochs-latest img/
  cp -r ${SRC_DIR}/msrs.def img/

  MakeDir ${PUBLISH_DIR}

  cd img
  tar cf ${PUBLISH_DIR}/img.tar .
  cd ..

  LogExecute cp ${START_DIR}/bochs.html ${PUBLISH_DIR}
  LogExecute cp ${BUILD_DIR}/bochs ${PUBLISH_DIR}/bochs_${NACL_ARCH}${NACL_EXEEXT}

  if [ ${NACL_ARCH} = pnacl ]; then
    sed -i.bak 's/x-nacl/x-pnacl/g' ${PUBLISH_DIR}/bochs.html
  fi

  pushd ${PUBLISH_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      bochs*${NACL_EXEEXT} \
      -s . \
      -o bochs.nmf
  popd
}

# $1 - url
# $2 - filename
# $3 - sha1
ImageDownloadStep() {
  cd ${NACL_PACKAGES_CACHE}
  # if matching tarball already exists, don't download again
  if ! CheckHash $2 $3; then
    Fetch $1 $2
    if ! CheckHash $2 $3 ; then
       Banner "$2 failed checksum!"
       exit -1
    fi
  fi
}

DownloadStep() {
  ImageDownloadStep ${LINUX_IMG_URL} ${LINUX_IMG_NAME}.tar.gz ${LINUX_IMG_SHA1}
  ImageExtractStep ${LINUX_IMG_NAME}.tar.gz ${LINUX_IMG_NAME}
}
