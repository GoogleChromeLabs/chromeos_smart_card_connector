#!/bin/bash
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_CPPFLAGS+=" -I${NACLPORTS_INCLUDE}/pixman-1"
NACLPORTS_CPPFLAGS+=" -DFASYNC=O_NONBLOCK -DFNDELAY=O_NONBLOCK"

EXECUTABLES=hw/kdrive/sdl/Xsdl${NACL_EXEEXT}

EnableGlibcCompat

EXTRA_CONFIGURE_ARGS+=" --disable-glx"
EXTRA_CONFIGURE_ARGS+=" --enable-xfree86-utils=no"
EXTRA_CONFIGURE_ARGS+=" --enable-pciaccess=no"
EXTRA_CONFIGURE_ARGS+=" --enable-kdrive"
EXTRA_CONFIGURE_ARGS+=" --enable-local-transport=no"
EXTRA_CONFIGURE_ARGS+=" --enable-unix-transport=no"
EXTRA_CONFIGURE_ARGS+=" --enable-static=yes"
EXTRA_CONFIGURE_ARGS+=" --enable-shared=no"
EXTRA_CONFIGURE_ARGS+=" --enable-unit-tests=no"
EXTRA_CONFIGURE_ARGS+=" --enable-ipv6=no"
EXTRA_CONFIGURE_ARGS+=" --datarootdir=/share"
EXTRA_CONFIGURE_ARGS+=" --with-fontrootdir=/share/fonts/X11"
EXTRA_CONFIGURE_ARGS+=" --with-xkb-bin-directory="

if [ "${NACL_LIBC}" = "newlib" ]; then
  EXTRA_CONFIGURE_ARGS+=" --enable-xvfb=no"
  EXTRA_CONFIGURE_ARGS+=" --enable-xorg=no"
fi

NACLPORTS_CPPFLAGS+=" -Dmain=SDL_main"
export LIBS="\
${NACL_CLI_MAIN_LIB} \
-Wl,--undefined=SDL_main \
-Wl,--undefined=nacl_main \
-Wl,--undefined=nacl_startup_untar \
-lSDLmain -lSDL -lRegal -lglslopt -lppapi_gles2 -lm \
-l${NACL_CXX_LIB}"

PatchStep() {
  DefaultPatchStep
  MakeDir ${SRC_DIR}/hw/kdrive/sdl
  LogExecute cp ${START_DIR}/sdl.c \
                ${START_DIR}/Makefile.am \
                ${SRC_DIR}/hw/kdrive/sdl/
}

ConfigureStep() {
  ChangeDir ${SRC_DIR}
  autoreconf --force -v --install
  # Without this xorg-server's configure script will run -print-program-path=ld
  # which gives the wrong anser for PNaCl
  export LD=${NACLLD}
  export GL_CFLAGS=" "
  export GL_LIBS="-lRegal"
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

InstallStep() {
  Banner "Publishing to ${PUBLISH_DIR}"
  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR=${PUBLISH_DIR}/xorg-server
  MakeDir ${ASSEMBLY_DIR}

  ChangeDir ${ASSEMBLY_DIR}
  LogExecute cp ${BUILD_DIR}/hw/kdrive/sdl/Xsdl${NACL_EXEEXT} \
                ${ASSEMBLY_DIR}/Xsdl_${NACL_ARCH}${NACL_EXEEXT}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${ASSEMBLY_DIR}/Xsdl_*${NACL_EXEEXT} \
      -s . \
      -o Xsdl.nmf

  # Bash is already platform specific split, copy the whole thing.
  local BASH_DIR=${NACL_PACKAGES_PUBLISH}/bash/${TOOLCHAIN}/bash_multiarch
  LogExecute cp -fR ${BASH_DIR}/* ${ASSEMBLY_DIR}

  LogExecute cp ${NACLPORTS_BIN}/xkbcomp${NACL_EXEEXT} \
      ${ASSEMBLY_DIR}/xkbcomp_${NACL_ARCH}${NACL_EXEEXT}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${ASSEMBLY_DIR}/xkbcomp_*${NACL_EXEEXT} \
      -s . \
      -o xkbcomp.nmf

  # Install the HTML/JS for the terminal.
  LogExecute python ${TOOLS_DIR}/create_term.py -i whitelist.js Xsdl.nmf
  InstallNaClTerm ${ASSEMBLY_DIR}

  for f in manifest.json \
           background.js \
           xorg_16.png \
           xorg_48.png \
           xorg_128.png \
           whitelist.js \
           Xsdl.js; do
    LogExecute cp ${START_DIR}/${f} .
  done

  ChangeDir ${NACL_PREFIX}
  LogExecute tar cvf ${ASSEMBLY_DIR}/xorg-xkb.tar share/X11/xkb
  LogExecute shasum ${ASSEMBLY_DIR}/xorg-xkb.tar > \
      ${ASSEMBLY_DIR}/xorg-xkb.tar.hash
  local XFONTS_DIR=${NACL_PACKAGES_PUBLISH}/xfonts/${TOOLCHAIN}
  LogExecute cp ${XFONTS_DIR}/xorg-fonts.tar ${ASSEMBLY_DIR}/
  LogExecute cp ${XFONTS_DIR}/xorg-fonts.tar.hash ${ASSEMBLY_DIR}/

  ChangeDir ${PUBLISH_DIR}
  LogExecute zip -r xorg-server.zip xorg-server
}
