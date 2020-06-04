#!/bin/bash
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is for bash on NaCl. Note that you cannot use external commands
# until the installation is completed. Also, you cannot use features
# which nacl_io does not support yet (e.g., pipes and sub-shells).

set -e

CheckNaClEnabled() {
  # Skip check on if this isn't newlib.
  if [[ ${TOOLCHAIN} != newlib ]]; then
    return
  fi
  TMP_CHECK_FILE="/tmp/.enable_nacl_check.nexe"
  # Assume we can reuse the test file if present.
  if [[ ! -e ${TMP_CHECK_FILE} ]]; then
    geturl -q _platform_specific/${NACL_ARCH}/bash.nexe \
      ${TMP_CHECK_FILE} || exit 1
  fi
  set +e
  ${TMP_CHECK_FILE} -c 'exit 42'
  if [[ $? != 42 ]]; then
    echo "*********************** ERROR ***********************"
    echo
    echo "In order to use the NaCl Dev Environment, you must"
    echo "currently enable 'Native Client' at the url:"
    echo "  chrome://flags"
    echo "You must then restart your browser."
    echo
    echo "Eventually this should not be required."
    echo "Follow this issue: https://crbug.com/477808"
    echo
    echo "*********************** ERROR ***********************"
    # TODO: A more proper way to handle error would be "exit 1" here
    # and keep window open so that error message could be shown.
    while [[ 1 == 1 ]]; do
      read
    done
  fi
  set -e
}

InstallBasePackages() {
  echo "===> Installing core packages"
  local core_packages="
    coreutils \
    bash \
    curl \
    git \
    make"

  if [[ -f /usr/etc/pkg/repos/NaCl.conf ]]; then
    pkg install -y $core_packages
  else
    if [ "${NACL_DEVENV_LOCAL:-}" = "1" ]; then
      local repos_dir=/mnt/http/repos_local_${NACL_ARCH}
    else
      local repos_dir=/mnt/http/repos_${NACL_ARCH}
    fi
    pkg -R $repos_dir install -y $core_packages

    echo "===> Setting up pkg"
    # Now that we have coreutils installed we can copy the pkg config
    # files into place with 'cp'
    mkdir -p /usr/etc/pkg/repos
    rm -f /usr/etc/pkg/repos/NaCl.conf
    cp $repos_dir/NaCl.conf /usr/etc/pkg/repos/
    pkg update
  fi

  echo "===> Installing extra packages"
  local extra_packages="
    findutils \
    grep \
    less \
    nano \
    python \
    emacs \
    vim"
  pkg install -y -U $extra_packages

  local have_gcc=0
  if [[ ${TOOLCHAIN} == glibc ]]; then
    if [[ ${NACL_ARCH} == i686 || ${NACL_ARCH} == x86_64 ]]; then
      echo "===> Installing compiler packages"
      dev_packages+=" \
    binutils \
    gcc \
    mingn"
      have_gcc=1
      pkg install -y -U $dev_packages
    fi
  fi

  if [[ ${have_gcc} == 0 ]]; then
    echo "WARNING: \
emacs and gcc not yet available for your platform (coming soon)."
  fi
}

CheckNaClEnabled
InstallBasePackages $@
