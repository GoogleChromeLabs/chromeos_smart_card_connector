#!/bin/bash
# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Installs required python modules using 'pip'.
# Always use a locally installed version of pip rather than relying
# on the system version since the behaviour and command line flags
# for pip inself vary between versions.

SCRIPT_DIR="$(cd $(dirname $0) && pwd)"
ARGS="--no-compile"

cd "${SCRIPT_DIR}/.."

source ../../python2_venv/bin/activate

# At this point we know we have good pip install in $PATH and we can use
# it to install the requirements.
pip install ${ARGS} -r requirements.txt
