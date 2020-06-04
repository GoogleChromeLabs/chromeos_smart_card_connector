# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=hello${NACL_EXEEXT}

TestStep() {
  if [ "${TOOLCHAIN}" = "pnacl" ]; then
    return
  fi
  LogExecute ./hello
}
