# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Workaround for arm-gcc bug:
# https://code.google.com/p/nativeclient/issues/detail?id=3205
# TODO(sbc): remove this once the issue is fixed
if [ "${NACL_ARCH}" = "arm" ]; then
  NACLPORTS_CPPFLAGS+=" -mfpu=vfp"
fi

NACLPORTS_CPPFLAGS+=" -Dmain=nacl_main"
NACLPORTS_LIBS+=" ${NACL_CLI_MAIN_LIB}"

EXECUTABLES="test/cairo-test-suite${NACL_EXEEXT}"

# This is only necessary for pnacl
export ax_cv_c_float_words_bigendian=no

# Enable the x backend
EXTRA_CONFIGURE_ARGS+=" --with-x"
EXTRA_CONFIGURE_ARGS+=" --enable-xlib=yes"
EXTRA_CONFIGURE_ARGS+=" --enable-xlib-xrender=yes"
# disable xcb
EXTRA_CONFIGURE_ARGS+=" --disable-xcb"
EXTRA_CONFIGURE_ARGS+=" --disable-xlib-xcb"
EXTRA_CONFIGURE_ARGS+=" --disable-xcb-shm"
# gobjects required for gdk-pixbuf -> gtk+
EXTRA_CONFIGURE_ARGS+=" --enable-gobject=yes"
EXTRA_CONFIGURE_ARGS+=" --enable-ft"

export ac_cv_func_XRenderCreateLinearGradient=yes
export ac_cv_func_XRenderCreateRadialGradient=yes
export ac_cv_func_XRenderCreateConicalGradient=yes
