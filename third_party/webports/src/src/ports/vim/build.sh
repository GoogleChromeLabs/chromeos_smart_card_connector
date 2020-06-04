# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EXTRA_CONFIGURE_ARGS+="--enable-gui=no --with-tlib=ncurses"
EXTRA_CONFIGURE_ARGS+=" --prefix=/usr --exec-prefix=/usr"
EXECUTABLES=src/vim${NACL_EXEEXT}
export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"

PatchStep() {
  DefaultPatchStep
  LogExecute cp ${START_DIR}/vim_pepper.c ${SRC_DIR}/src/vim_pepper.c
}

ConfigureStep() {
  # These settings are required by vim's configure when cross compiling.
  # These are the standard valued detected when configuring for linux/glibc.
  export vim_cv_toupper_broken=no
  export vim_cv_terminfo=yes
  export vim_cv_tty_mode=0620
  export vim_cv_tty_group=world
  export vim_cv_getcwd_broken=no
  export vim_cv_stat_ignores_slash=no
  export vim_cv_memmove_handles_overlap=yes
  export ac_cv_func_getrlimit=no
  if [ "${NACL_DEBUG}" == "1" ]; then
    export STRIP=echo
  else
    export STRIP=${NACLSTRIP}
  fi
  DefaultConfigureStep
  # Vim's build doesn't support building outside the source tree.
  # Do a clean to make rebuild after failure predictable.
  LogExecute make clean
}

InstallStep() {
  DefaultInstallStep
  LogExecute mv ${INSTALL_DIR}/usr ${INSTALL_DIR}${PREFIX}
}

PublishStep() {
  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR="${PUBLISH_DIR}/vim"

  MakeDir ${ASSEMBLY_DIR}/vimtar/usr
  ChangeDir ${ASSEMBLY_DIR}/vimtar
  LogExecute cp -a ${INSTALL_DIR}${PREFIX}/* ./usr/
  LogExecute cp usr/bin/vim${NACL_EXEEXT} ../vim_${NACL_ARCH}${NACL_EXEEXT}
  LogExecute cp $SRC_DIR/runtime/vimrc_example.vim usr/share/vim/vimrc
  LogExecute rm -rf usr/bin
  LogExecute rm -rf usr/share/man
  tar cf ${ASSEMBLY_DIR}/vim.tar .
  Remove ${ASSEMBLY_DIR}/vimtar
  shasum ${ASSEMBLY_DIR}/vim.tar > ${ASSEMBLY_DIR}/vim.tar.hash
  cd ${ASSEMBLY_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      vim_*${NACL_EXEEXT} \
      -s . \
      -o vim.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py vim.nmf

  GenerateManifest ${START_DIR}/manifest.json ${ASSEMBLY_DIR}
  InstallNaClTerm ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/background.js ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/vim.html ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/vim_app.html ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/vim.js ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_16.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_48.png ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/icon_128.png ${ASSEMBLY_DIR}
  ChangeDir ${PUBLISH_DIR}
  CreateWebStoreZip vim-${VERSION}.zip vim
}
