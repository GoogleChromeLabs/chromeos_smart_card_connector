# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
  NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LIB}"
  EXTRA_CONFIGURE_ARGS+=" --with-x11"

  export gio_can_sniff=no
  DefaultConfigureStep
}
