# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Use the same patch here as for the destination tree.  This allows us to use
# the same tree for both, although some care should be taken to note that our
# DYNLOAD will be different between the two builds.
BUILD_DIR=${WORK_DIR}/build_host

ConfigureStep() {
  # Reset CFLAGS and LDFLAGS when configuring the host
  # version of python since they hold values designed for
  # building for NaCl.  Note that we are forcing 32 bits here so
  # our generated modules use Py_ssize_t of the correct size.
  export CC="gcc -m32"
  export CXX="g++ -m32"
  export LD="gcc -m32"
  LogExecute ${SRC_DIR}/configure --prefix=${NACL_HOST_PYROOT}
}

BuildStep() {
  DefaultBuildStep
  ssl_lib=$(find build -name "_ssl*")
  if [ -z "${ssl_lib}" ]; then
    echo "Failed to build _ssl python module."
    echo "Check for 32-bit install of libssl and libcryto (see README.rst)"
    exit 1
  fi
}

InstallStep() {
  make install
}
