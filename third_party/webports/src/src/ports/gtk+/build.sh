# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  if [ "${NACL_LIBC}" = "newlib" ]; then
    # newlib requires different library order to deal with static libraries
    NACLPORTS_LIBS+=" -lXext -lX11 -lxcb -lXau"
  fi

  EnableGlibcCompat

  EXTRA_CONFIGURE_ARGS+=" --disable-shm --enable-explicit-deps --disable-cups \
   --enable-gtk-doc-html=no"

  NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main -pthread"
  NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"
  export enable_gtk_doc=no

  DefaultConfigureStep
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}
  local FONT_ROOT_DIR="${PUBLISH_DIR}/${NACL_ARCH}/root"
  local FONTCONFIG_DIR="${START_DIR}/../fontconfig"
  local FONT_CACHE="${NACL_PACKAGES_PUBLISH}/xfonts/${TOOLCHAIN}/${NACL_ARCH}"

  # Create Fonts tar

  # Setup Directory Structure
  # TODO(josephpereira): put everything in /usr not naclports-dummydir
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/var/cache/fontconfig
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/lib/pkgconfig
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/share/gtk-2.0/demo
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/share/themes
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/share/aclocal
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/share/X11
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/share/locale
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/lib/gdk-pixbuf-2.0/2.10.0
  MakeDir ${FONT_ROOT_DIR}/naclports-dummydir/etc/pango
  MakeDir ${FONT_ROOT_DIR}/usr/share/locale
  MakeDir ${FONT_ROOT_DIR}/usr/share/fonts
  MakeDir ${FONT_ROOT_DIR}/usr/lib/pkgconfig
  MakeDir ${FONT_ROOT_DIR}/gdk-pixbuf
  MakeDir ${FONT_ROOT_DIR}/modules
  MakeDir ${FONT_ROOT_DIR}/conf.d

  # Move fontconfig files
  LogExecute cp -rf ${NACLPORTS_LIBDIR}/../share/fontconfig/conf.avail/*\
   ${FONT_ROOT_DIR}/conf.d
  LogExecute cp -rf ${FONTCONFIG_DIR}/fonts\
   ${FONT_ROOT_DIR}/naclports-dummydir/etc

  # Get Cache from xfonts
  LogExecute cp -f ${FONT_CACHE}/xorg-fonts-cache-${NACL_ARCH}.tar\
   ${FONT_ROOT_DIR}/cache.tar
  ChangeDir ${FONT_ROOT_DIR}
  LogExecute tar -xvf cache.tar
  LogExecute rm -f cache.tar

  # Move X11 font files
  LogExecute cp -rf ${NACLPORTS_LIBDIR}/../share/fonts/*\
   ${FONT_ROOT_DIR}/usr/share/fonts
  LogExecute cp -rf ${NACLPORTS_LIBDIR}/../share/X11/locale/*\
   ${FONT_ROOT_DIR}/usr/share/locale

  # Other setup
  echo "0" > ${FONT_ROOT_DIR}/usr/share/fonts/X11/fonts.scale
  echo "0" > ${FONT_ROOT_DIR}/usr/share/fonts/X11/fonts.dir
  echo "[Pango]" > ${FONT_ROOT_DIR}/naclports-dummydir/etc/pango/pangorc
  echo "ModuleFiles = ../modules/pango.modules" >>\
   ${FONT_ROOT_DIR}/naclports-dummydir/etc/pango/pangorc
  LogExecute touch ${FONT_ROOT_DIR}/naclports-dummydir/lib/charset.alias
  LogExecute cp -f ${NACLPORTS_LIBDIR}/pkgconfig/gdk-pixbuf*.pc\
   ${FONT_ROOT_DIR}/naclports-dummydir/lib/pkgconfig
  LogExecute cp -f ${NACLPORTS_LIBDIR}/pkgconfig/gdk-pixbuf*.pc\
   ${FONT_ROOT_DIR}/usr/lib/pkgconfig
  LogExecute touch\
   ${FONT_ROOT_DIR}/naclports-dummydir/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
  LogExecute cp -rf ${NACLPORTS_LIBDIR}/../share/aclocal/*\
   ${FONT_ROOT_DIR}/naclports-dummydir/share/aclocal
  LogExecute cp -rf ${NACLPORTS_LIBDIR}/../share/locale/*\
   ${FONT_ROOT_DIR}/naclports-dummydir/share/locale
  LogExecute cp -rf ${NACLPORTS_LIBDIR}/../share/X11/*\
   ${FONT_ROOT_DIR}/naclports-dummydir/share/X11

  # Looks for these files to be there, otherwise assumes bad install
  LogExecute touch ${FONT_ROOT_DIR}/gdk-pixbuf/gdk-pixbuf.loaders
  LogExecute touch ${FONT_ROOT_DIR}/modules/pango.modules

  # Grab source to display in demo
  LogExecute cp -rf ${BUILD_DIR}/../gtk+-2.24.28/demos/gtk-demo/*\
   ${FONT_ROOT_DIR}/naclports-dummydir/share/gtk-2.0/demo

  # tar gtk+ directory structure
  LogExecute tar -cvf gtk-demo.tar *

  local APP_DIR="${PUBLISH_DIR}/${NACL_ARCH}"
  MakeDir ${APP_DIR}
  LogExecute cp -f gtk-demo.tar ${APP_DIR}/gtk-demo.tar
  ChangeDir ${APP_DIR}
  local exe="${APP_DIR}/gtk-demo${NACL_EXEEXT}"
  LogExecute cp ${INSTALL_DIR}/naclports-dummydir/bin/gtk-demo${NACL_EXEEXT} \
   ${exe}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
          ${exe} -s . -L ${INSTALL_DIR}/naclports-dummydir/lib \
          -o gtk-demo.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py -n gtk-demo gtk-demo.nmf
  InstallNaClTerm ${APP_DIR}
  LogExecute cp -f ${START_DIR}/*.js ${APP_DIR}
  LogExecute rm -rf ${FONT_ROOT_DIR}
}
