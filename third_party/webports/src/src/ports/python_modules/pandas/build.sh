# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildStep() {
  DefaultPythonModuleBuildStep
  # This avoids name conflicts with Python's parser.o
  for OBJ in parser tokenizer; do
    LogExecute mv ${DEST_PYTHON_OBJS}/${PACKAGE_NAME}/${OBJ}.o \
                  ${DEST_PYTHON_OBJS}/${PACKAGE_NAME}/p_${OBJ}.o
  done
}

InstallStep() {
  DefaultPythonModuleInstallStep
}

ConfigureStep() {
  return
}
