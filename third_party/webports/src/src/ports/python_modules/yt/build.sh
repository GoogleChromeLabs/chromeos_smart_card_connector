# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  export PNG_DIR=${NACL_DEST_PYROOT}
  DefaultPythonModuleBuildStep
}

InstallStep() {
  DefaultPythonModuleInstallStep
}

ConfigureStep() {
  return
}
