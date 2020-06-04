# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  EXTRA_CONFIGURE_ARGS+=" --disable-python --without-dbus --without-webkit"
  EXTRA_CONFIGURE_ARGS+=" --with-gnu-ld"
  NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
  NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"

  DefaultConfigureStep
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}
  local APP_DIR="${PUBLISH_DIR}/${NACL_ARCH}"
  local TAR_ROOT="${APP_DIR}/root"
  MakeDir ${TAR_ROOT}
  MakeDir ${APP_DIR}
  ChangeDir ${APP_DIR}
  local exe="${APP_DIR}/gimp${NACL_EXEEXT}"
  LogExecute cp ${INSTALL_DIR}/naclports-dummydir/bin/gimp-2.8${NACL_EXEEXT} \
   ${exe}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
          ${exe} -s . -L ${INSTALL_DIR}/naclports-dummydir/lib \
          -o gimp.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py -n gimp gimp.nmf
  InstallNaClTerm ${APP_DIR}
  LogExecute cp -f ${START_DIR}/*.js ${APP_DIR}

  # take the gtk+ fonts and add some additional rc files that gimp requires
  LogExecute cp -f \
  ${NACL_PACKAGES_PUBLISH}/gtk+/${TOOLCHAIN}/${NACL_ARCH}/gtk-demo.tar\
   ${TAR_ROOT}/gtk-demo.tar
  ChangeDir ${TAR_ROOT}
  LogExecute tar -xvf gtk-demo.tar
  LogExecute rm -f gtk-demo.tar
  LogExecute cp -rf ${INSTALL_DIR}/naclports-dummydir/etc/*\
   ./naclports-dummydir/etc
  LogExecute cp -rf ${INSTALL_DIR}/naclports-dummydir/share/*\
   ./naclports-dummydir/share
  LogExecute rm -Rf ./naclports-dummydir/share/man

  LogExecute tar -cvf gimp.tar *
  LogExecute cp gimp.tar ${APP_DIR}/gimp.tar
  shasum ${APP_DIR}/gimp.tar > ${APP_DIR}/gimp.tar.hash
  LogExecute rm -rf ${TAR_ROOT}
}
