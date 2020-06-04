# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXTRA_CONFIGURE_ARGS="--disable-assembly --disable-pthread-sem"

AutogenStep() {
  ChangeDir ${SRC_DIR}
  # For some reason if we don't remove configure before running
  # autoconf it doesn't always get updates correctly.  About half
  # the time the old configure script (with no reference to nacl)
  # will remain after ./autogen.sh
  rm -f configure
  ./autogen.sh
  PatchConfigure
  PatchConfigSub
  cd -
}

ConfigureStep() {
  AutogenStep
  SetupCrossEnvironment
  DefaultConfigureStep
}
