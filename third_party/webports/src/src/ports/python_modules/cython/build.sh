# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  ChangeDir ${SRC_DIR}
  # No destination installation
  LogExecute rm -rf build dist
  export NACL_PORT_BUILD=host
  # Force 32 bits here for Py_ssize_t correctness.
  export CC="gcc -m32"
  export CXX="g++ -m32"
  export LD="gcc -m32"
  LogExecute ${NACL_HOST_PYTHON} setup.py \
    ${NACL_PYSETUP_ARGS:-} \
    install --prefix=${NACL_HOST_PYROOT}
}

InstallStep() {
  return
}

ConfigureStep() {
  return
}
