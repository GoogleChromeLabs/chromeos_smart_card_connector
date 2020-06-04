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

  local GTK_PORT_DIR=${START_DIR}/../gtk+
  local GTK_DIR=${NACL_PACKAGES_PUBLISH}/gtk+/${TOOLCHAIN}/${NACL_ARCH}
  local XORG_DIR=${NACL_PACKAGES_PUBLISH}/xorg-server/${TOOLCHAIN}/xorg-server
  local TWM_DIR=${NACL_PACKAGES_PUBLISH}/twm/${TOOLCHAIN}

  LogExecute cp ${GTK_DIR}/*.nmf ${ASSEMBLY_DIR}/
  LogExecute cp ${GTK_DIR}/*.nexe ${ASSEMBLY_DIR}/
  LogExecute cp -r ${GTK_DIR}/lib* ${ASSEMBLY_DIR}/
  LogExecute cp ${GTK_DIR}/*.tar ${ASSEMBLY_DIR}/

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

  LogExecute cp ${START_DIR}/Xsdl.js ${ASSEMBLY_DIR}/
  LogExecute cp ${START_DIR}/*.png ${ASSEMBLY_DIR}/

  LogExecute python ${TOOLS_DIR}/create_term.py -n gtk-demo gtk-demo.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py -n twm twm.nmf
  GenerateManifest ${START_DIR}/manifest.json ${ASSEMBLY_DIR} \
    "TITLE"="Gtk-Demo"
}
