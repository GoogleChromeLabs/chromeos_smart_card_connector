# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

OS_JOBS=1

export RUNPROGRAM="${NACL_SDK_ROOT}/tools/sel_ldr.py"

EXTRA_CONFIGURE_ARGS+=" --prefix=/usr --exec-prefix=/usr"

if [ ${EMACS_X:-0} = 1 ]; then
  EXTRA_CONFIGURE_ARGS+=" --with-x"
  EXTRA_CONFIGURE_ARGS+=" --with-x-toolkit=athena"
  EXTRA_CONFIGURE_ARGS+=" --with-xpm=no"
  EXTRA_CONFIGURE_ARGS+=" --with-jpeg=yes"
  EXTRA_CONFIGURE_ARGS+=" --with-png=yes"
  EXTRA_CONFIGURE_ARGS+=" --with-gif=yes"
  EXTRA_CONFIGURE_ARGS+=" --with-tiff=yes"
else
  EXTRA_CONFIGURE_ARGS+=" --with-x=no"
  EXTRA_CONFIGURE_ARGS+=" --with-xpm=no"
  EXTRA_CONFIGURE_ARGS+=" --with-jpeg=no"
  EXTRA_CONFIGURE_ARGS+=" --with-png=no"
  EXTRA_CONFIGURE_ARGS+=" --with-gif=no"
  EXTRA_CONFIGURE_ARGS+=" --with-tiff=no"
fi

export COMPAT_LIBS="-l${NACL_CXX_LIB}"

export EXTRA_LIBS="-ltar ${NACL_CLI_MAIN_LIB}"

if [ "${NACL_LIBC}" = "newlib" ]; then
  export emacs_cv_func__setjmp=no
  # Additional transitive dependencies not usually on the link line,
  # but required for static linking.
  COMPAT_LIBS="-pthread ${COMPAT_LIBS}"
  if [ ${EMACS_X:-0} = 1 ]; then
    COMPAT_LIBS="-lxcb -lXau -lXpm ${COMPAT_LIBS}"
  fi
  COMPAT_LIBS="-lnacl_io ${COMPAT_LIBS}"
  COMPAT_LIBS+=" -lglibc-compat"
  NACLPORTS_LIBS+=" ${COMPAT_LIBS}"
fi

EnableGlibcCompat

ConfigureStep() {
  export CFLAGS="${NACLPORTS_CFLAGS} -I${NACL_SDK_ROOT}/include"
  DefaultConfigureStep
}

# Build twice to workaround a problem in the build script that builds something
# partially the first time that makes the second time succeed.
# TODO(petewil): Find and fix the problem that makes us build twice.
BuildStep() {
  # Since we can't detect that a rebuild file hasn't changed, delete them all.
  # Rebuild a second time on the buildbots only.
  if [ "${BUILDBOT_BUILDERNAME:-}" != "" ]; then
    DefaultBuildStep || DefaultBuildStep
  else
    DefaultBuildStep
  fi
}

PatchStep() {
  DefaultPatchStep

  ChangeDir ${SRC_DIR}
  rm -f lisp/emacs-lisp/bytecomp.elc
  rm -f lisp/files.elc
  rm -f lisp/international/quail.elc
  rm -f lisp/startup.elc
  rm -f lisp/loaddefs.el
  rm -f lisp/loaddefs.elc
  rm -f lisp/vc/vc-git.elc
  LogExecute cp ${START_DIR}/emacs_pepper.c ${SRC_DIR}/src/emacs_pepper.c
}

InstallStep() {
  # Today the install step copies emacs_x86_64.nexe to the publish dir, but we
  # need nacl_temacs.nexe instead.  Change to copy that here.
  DefaultInstallStep
  LogExecute mv ${INSTALL_DIR}/usr ${INSTALL_DIR}${PREFIX}
  LogExecute cp ${BUILD_DIR}/src/nacl_temacs${NACL_EXEEXT} \
      ${DESTDIR}${PREFIX}/bin/emacs${NACL_EXEEXT}
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR="${PUBLISH_DIR}/emacs"

  if [ ${EMACS_X:-0} = 1 ]; then
    EMACS_NAME="EmacsXWindows"
  else
    EMACS_NAME="Emacs"
  fi

  # Copy all installed files into tarball
  MakeDir ${ASSEMBLY_DIR}/emacstar/usr
  ChangeDir ${ASSEMBLY_DIR}/emacstar
  LogExecute cp -a ${DESTDIR}${PREFIX}/* ./usr
  LogExecute cp usr/bin/emacs${NACL_EXEEXT} \
       ../emacs_${NACL_ARCH}${NACL_EXEEXT}
  rm -rf usr/bin
  rm -rf usr/share/man
  find . -iname "*.nexe" -delete
  mkdir -p ${ASSEMBLY_DIR}/emacstar/home/user/.emacs.d
  tar cf ${ASSEMBLY_DIR}/emacs.tar .
  rm -rf ${ASSEMBLY_DIR}/emacstar
  shasum ${ASSEMBLY_DIR}/emacs.tar > ${ASSEMBLY_DIR}/emacs.tar.hash
  cd ${ASSEMBLY_DIR}
  # TODO(petewil) this is expecting an exe, but we give it a shell script
  # since we have emacs running "unpacked", so it fails. Give it
  # nacl_temacs.nexe instead.
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      emacs_*${NACL_EXEEXT} \
      -s . \
      -o emacs.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py emacs.nmf
  InstallNaClTerm ${ASSEMBLY_DIR}
  GenerateManifest ${START_DIR}/manifest.json \
    ${ASSEMBLY_DIR} "TITLE"="${EMACS_NAME}"

  LogExecute cp ${START_DIR}/background.js ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_16.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_48.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_128.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/emacs.js ${ASSEMBLY_DIR}
  if [ ${EMACS_X:-0} = 1 ]; then
    local XORG_DIR=${NACL_PACKAGES_PUBLISH}/xorg-server/${TOOLCHAIN}/xorg-server
    LogExecute cp -r ${XORG_DIR}/_platform_specific ${ASSEMBLY_DIR}/
    LogExecute cp ${XORG_DIR}/*.nmf ${ASSEMBLY_DIR}/
    LogExecute cp ${XORG_DIR}/*.nexe ${ASSEMBLY_DIR}/
    if [[ ${NACL_SHARED} == 1 ]]; then
      LogExecute cp -r ${XORG_DIR}/lib* ${ASSEMBLY_DIR}/
    fi
    LogExecute cp ${XORG_DIR}/*.tar ${ASSEMBLY_DIR}/
    LogExecute cp ${XORG_DIR}/*.js ${ASSEMBLY_DIR}/
    LogExecute cp ${XORG_DIR}/*.html ${ASSEMBLY_DIR}/

    LogExecute cp ${START_DIR}/../emacs-x/Xsdl.js ${ASSEMBLY_DIR}/
  fi
  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip emacs-${VERSION}.zip emacs
}
