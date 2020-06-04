# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_CPPFLAGS+=" -Dpipe=nacl_spawn_pipe"

PatchStep() {
  LogExecute chmod 777 ${SRC_DIR}/configure
  DefaultPatchStep
}
