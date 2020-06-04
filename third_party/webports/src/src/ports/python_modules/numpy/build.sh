# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACL_PYSETUP_ARGS="build --fcompiler=fake"

BuildStep() {
  export ATLAS=None
  export LAPACK=None
  export BLAS=None
  ChangeDir ${SRC_DIR}
  LogExecute rm -rf build dist
  export NACL_PORT_BUILD=host
  # Force 32 bits here for Py_ssize_t correctness.
  export CC="gcc -m32 -msse"
  export CXX="g++ -m32 -msse"
  export LD="gcc -m32 -msse"
  LogExecute ${NACL_HOST_PYTHON} setup.py \
    ${NACL_PYSETUP_ARGS} \
    install --prefix=${NACL_HOST_PYROOT}
  DefaultPythonModuleBuildStep bootstrap
}

InstallStep() {
  DefaultPythonModuleInstallStep
}

ConfigureStep() {
  return
}
