# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXTRA_CONFIGURE_ARGS+=" --with-x"
EXTRA_CONFIGURE_ARGS+=" --x-includes=${NACLPORTS_INCLUDE}"
EXTRA_CONFIGURE_ARGS+=" --x-libraries=${NACLPORTS_LIBDIR}"

EXECUTABLES="src/blackbox util/bsetroot util/bstyleconvert"

export EXTRA_LIBS+="-liconv \
  -lXext -lXmu -lSM -lICE -lXt -lX11 -lxcb -lXau ${NACL_CLI_MAIN_LIB}"

# TODO(bradnelson): Find a better general pattern for this.
# Repeat these here to include them which checking for X11 and linking libxcb,
# which references ki_fcntl directly.
NACLPORTS_LIBS+=" -lnacl_io -pthread"

EnableGlibcCompat
