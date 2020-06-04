# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_CFLAGS+=" -std=gnu99"

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

  local conf_host=${NACL_CROSS_PREFIX}
  if [ ${NACL_ARCH} = "pnacl" ]; then
    # The PNaCl tools use "pnacl-" as the prefix, but config.sub
    # does not know about "pnacl".  It only knows about "le32-nacl".
    # Unfortunately, most of the config.subs here are so old that
    # it doesn't know about that "le32" either.  So we just say "nacl".
    conf_host="nacl-pnacl"
  fi

  LogExecute ${SRC_DIR}/configure \
    --host=${conf_host} \
    --prefix=${PREFIX} \
    --disable-assembly \
    --disable-pthread-sem
}
