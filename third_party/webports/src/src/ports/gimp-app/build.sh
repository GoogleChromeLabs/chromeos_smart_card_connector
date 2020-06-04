# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  # Meta package, no build.
  return
}

InstallStep() {
  local ASSEMBLY_DIR=${PUBLISH_DIR}/app
  MakeDir ${ASSEMBLY_DIR}

  local GIMP_DIR=${NACL_PACKAGES_PUBLISH}/gimp/${TOOLCHAIN}/${NACL_ARCH}
  local XORG_DIR=${NACL_PACKAGES_PUBLISH}/xorg-server/${TOOLCHAIN}/xorg-server
  local TWM_DIR=${NACL_PACKAGES_PUBLISH}/twm/${TOOLCHAIN}

  LogExecute cp ${GIMP_DIR}/*.nmf ${ASSEMBLY_DIR}/
  LogExecute cp ${GIMP_DIR}/*.nexe ${ASSEMBLY_DIR}/
  LogExecute cp -r ${GIMP_DIR}/lib* ${ASSEMBLY_DIR}/

  MakeDir ${PUBLISH_DIR}/root
  ChangeDir ${PUBLISH_DIR}/root
  LogExecute cp ${GIMP_DIR}/*.tar .
  LogExecute tar -xvf *.tar
  LogExecute rm -f *.tar
  # make gimp fit the x screen in single-window-mode on startup
  LogExecute cp -f ${START_DIR}/sessionrc\
   ./naclports-dummydir/etc/gimp/2.0/sessionrc
  ChangeDir ${PUBLISH_DIR}/root
  LogExecute tar -cvf ${ASSEMBLY_DIR}/gimp.tar *
  LogExecute rm -Rf ${PUBLISH_DIR}/root
  ChangeDir ${ASSEMBLY_DIR}/

  LogExecute cp ${TWM_DIR}/*.nmf ${ASSEMBLY_DIR}/
  LogExecute cp ${TWM_DIR}/*.nexe ${ASSEMBLY_DIR}/
  LogExecute cp -r ${TWM_DIR}/lib* ${ASSEMBLY_DIR}/

  LogExecute cp -r ${XORG_DIR}/_platform_specific ${ASSEMBLY_DIR}/
  LogExecute cp ${XORG_DIR}/*.nmf ${ASSEMBLY_DIR}/
  LogExecute cp ${XORG_DIR}/*.nexe ${ASSEMBLY_DIR}/
  LogExecute cp -r ${XORG_DIR}/lib* ${ASSEMBLY_DIR}/
  LogExecute cp ${XORG_DIR}/*.tar ${ASSEMBLY_DIR}/
  LogExecute cp ${XORG_DIR}/*.js ${ASSEMBLY_DIR}/
  LogExecute cp ${XORG_DIR}/*.html ${ASSEMBLY_DIR}/

  LogExecute cp ${START_DIR}/*.js ${ASSEMBLY_DIR}/
  LogExecute cp ${START_DIR}/*.png ${ASSEMBLY_DIR}/

  LogExecute python ${TOOLS_DIR}/create_term.py -n gimp gimp.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py -n twm twm.nmf
  GenerateManifest ${START_DIR}/manifest.json ${ASSEMBLY_DIR} \
    "TITLE"="Gimp"
}
