# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"

EnableGlibcCompat

NACLPORTS_CPPFLAGS+=" -DGNULIB_defined_struct_sigaction -Dpipe=nacl_spawn_pipe"

PatchStep() {
  DefaultPatchStep
  # Touch documentation to prevent it from updating.
  touch ${SRC_DIR}/doc/*
}
