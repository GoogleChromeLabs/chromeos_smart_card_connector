# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

MIRROR_URL=http://storage.googleapis.com/naclports/mirror

# Beneath a Steel Sky (floppy version)
readonly BASS_FLOPPY_URL=${MIRROR_URL}/scummvm_games/bass/BASS-Floppy-1.3.zip
readonly BASS_FLOPPY_NAME=BASS-Floppy-1.3
readonly BASS_FLOPPY_SHA1=acd7a578879c73fb3994b9c72b6571dc62635f6e

readonly LURE_URL=${MIRROR_URL}/scummvm_games/lure/lure-1.1.zip
readonly LURE_NAME=lure-1.1
readonly LURE_SHA1=b7d430b17c361396ccf81734148cb0abb4bb4abb

EXECUTABLES=scummvm

SCUMMVM_EXAMPLE_DIR=${NACL_SRC}/ports/scummvm

ConfigureStep() {
  # NOTE: We can't use the DefaultConfigureStep, because the scummvm
  # configure script is hand-rolled, and won't accept additional arguments.
  # export the nacl tools
  # without this setting *make* will not show the full command lines
  export VERBOSE_BUILD=1
  SetupCrossEnvironment

  local conf_host=${NACL_CROSS_PREFIX}
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    conf_host="pnacl"
  elif [ "${NACL_ARCH}" = "arm" ]; then
    conf_host="nacl-arm"
  else
    conf_host="nacl-x86"
  fi
  export DEFINES="-DNACL -DSYSTEM_NOT_SUPPORTING_D_TYPE=1"

  # NOTE: disabled mt32emu because it using inline assembly that won't
  #     validate.
  ${SRC_DIR}/configure \
    --prefix=${PREFIX} \
    --host=${conf_host} \
    --disable-flac \
    --disable-zlib \
    --disable-mt32emu \
    --disable-timidity \
    --disable-all-engines \
    --enable-engine=lure \
    --enable-engine=sky
}

InstallStep() {
  SCUMMVM_DIR=runimage/usr/local/share/scummvm
  ChangeDir ${SRC_DIR}
  MakeDir ${SCUMMVM_DIR}
  cp gui/themes/scummclassic.zip \
     dists/engine-data/sky.cpt \
     gui/themes/scummmodern.zip \
     gui/themes/translations.dat \
     dists/engine-data/lure.dat \
     dists/pred.dic \
     ${SCUMMVM_DIR}

  cp $(find gui/themes/fonts/ -type f) ${SCUMMVM_DIR}

  Banner "Creating tarballs"
  cd runimage
  tar cf ../runimage.tar ./
  cd ..

  #Beneath a Steel Sky (Floppy version)
  BASS_DIR=bass/usr/local/share/scummvm/${BASS_FLOPPY_NAME}
  mkdir -p ${BASS_DIR}
  cp -r ${WORK_DIR}/${BASS_FLOPPY_NAME}/* ${BASS_DIR}
  cd bass
  tar cf ../bass.tar ./
  cd ..

  #Lure of the temptress
  LURE_DIR=lure/usr/local/share/scummvm
  mkdir -p ${LURE_DIR}
  cp -r ${WORK_DIR}/${LURE_NAME}/* ${LURE_DIR}
  cd lure
  tar cf ../lure.tar ./
  cd ..

  Banner "Publishing to ${PUBLISH_DIR}"
  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR=${PUBLISH_DIR}/scummvm

  MakeDir ${ASSEMBLY_DIR}

  cp ${START_DIR}/packaged_app/* ${ASSEMBLY_DIR}
  cp ${SRC_DIR}/*.tar ${ASSEMBLY_DIR}
  if [ "${NACL_DEBUG}" = "1" ]; then
    cp ${BUILD_DIR}/scummvm \
        ${ASSEMBLY_DIR}/scummvm_${NACL_ARCH}${NACL_EXEEXT}
  else
    ${NACLSTRIP} ${BUILD_DIR}/scummvm \
        -o ${ASSEMBLY_DIR}/scummvm_${NACL_ARCH}${NACL_EXEEXT}
  fi

  python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${ASSEMBLY_DIR}/scummvm_*${NACL_EXEEXT} \
      -s ${ASSEMBLY_DIR} \
      -o ${ASSEMBLY_DIR}/scummvm.nmf

  if [ "${NACL_ARCH}" = "pnacl" ]; then
    sed -i.bak 's/x-nacl/x-pnacl/' ${ASSEMBLY_DIR}/index.html
  fi

  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip scummvm-${VERSION}.zip scummvm
}

# $1 - url
# $2 - filename
# $3 - sha1
DownloadZipStep() {
  cd ${NACL_PACKAGES_CACHE}
  # if matching zip already exists, don't download again
  if ! CheckHash $2 $3; then
    Fetch $1 $2
    if ! CheckHash $2 $3; then
       Banner "$2 failed checksum!"
       exit -1
    fi
  fi
}

ExtractGameZipStep() {
  Banner "Unzipping ${2}"
  ChangeDir ${WORK_DIR}
  Remove ${1}
  unzip -d ${1} ${NACL_PACKAGES_CACHE}/${2}
}

# $1 - url
# $2 - filename
# $3 - sha1
GameGetStep() {
  DownloadZipStep $1 $2.zip $3
  ExtractGameZipStep $2 $2.zip
}

DownloadStep() {
  GameGetStep ${BASS_FLOPPY_URL} ${BASS_FLOPPY_NAME} ${BASS_FLOPPY_SHA1}
  GameGetStep ${LURE_URL} ${LURE_NAME} ${LURE_SHA1}
}
