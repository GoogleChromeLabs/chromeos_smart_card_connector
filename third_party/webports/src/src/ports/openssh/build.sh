# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="scp${NACL_EXEEXT} ssh${NACL_EXEEXT} \
             ssh-add${NACL_EXEEXT} sshd${NACL_EXEEXT}"
INSTALL_TARGETS="install-nokeys"

# Add --with-privsep-path otherwise openssh creates /var/empty
# in the root of DESTDIR.
EXTRA_CONFIGURE_ARGS="--with-privsep-path=${PREFIX}/var/empty"

# Force configure to recognise the existence of truncate
# and sigaction.  Normally it will detect that both this functions
# are implemented by glibc in terms of NOSYS.
export ac_cv_func_truncate=yes
export ac_cv_func_sigaction=yes

export SSHLIBS="${NACL_CLI_MAIN_LIB}"
if [ "${NACL_LIBC}" = "newlib" ]; then
  NACLPORTS_LIBS+=" -lcrypto"
  export LD="${NACLCXX}"
fi

if [ "${NACL_LIBC}" = "glibc" ]; then
  # The host version of 'strip' doesn't always recognise NaCl binaries
  # and ssh runs 'install -s' which doesn't always runs the host 'strip'
  EXTRA_CONFIGURE_ARGS+=" --disable-strip"
fi

EnableGlibcCompat

PublishStep() {
  DefaultInstallStep

  MakeDir ${PUBLISH_DIR}
  local ASSEMBLY_DIR="${PUBLISH_DIR}/openssh"
  MakeDir ${ASSEMBLY_DIR}
  LogExecute cp ssh${NACL_EXEEXT} \
      ${ASSEMBLY_DIR}/ssh_${NACL_ARCH}${NACL_EXEEXT}

  pushd ${ASSEMBLY_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ssh_*${NACL_EXEEXT} \
      -s . \
      -o openssh.nmf
  LogExecute python ${TOOLS_DIR}/create_term.py openssh.nmf
  popd

  InstallNaClTerm ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/background.js ${ASSEMBLY_DIR}
  LogExecute cp ${START_DIR}/manifest.json ${ASSEMBLY_DIR}
}
