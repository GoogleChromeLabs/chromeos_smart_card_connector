#!/bin/bash
# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Installs required python modules using 'pip'.
# Always use a locally installed version of pip rather than relying
# on the system version since the behaviour and command line flags
# for pip inself vary between versions.

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
ARGS="--user --no-compile"

cd "${SCRIPT_DIR}/.."

export PYTHONUSERBASE=$PWD/out/pip
pip_bin_dir=$PYTHONUSERBASE/bin
pip_bin=$pip_bin_dir/pip
export PATH=$pip_bin_dir:$PATH

if [ ! -f "$pip_bin" ]; then
  # On first run install pip directly from the network
  echo "Installing pip.."
  # Use local file rather than pipeline so we can detect failure of the curl
  # command.
  curl --silent --show-error https://bootstrap.pypa.io/get-pip.py > get-pip.py
  python get-pip.py --force-reinstall --user
  rm -f get-pip.py
  hash -r
fi

set -x
# Pin locally install pip to a specific version
pip install --user "pip==6.0.6"

# At this point we know we have good pip install in $PATH and we can use
# it to install the requirements.
pip install ${ARGS} -r requirements.txt
