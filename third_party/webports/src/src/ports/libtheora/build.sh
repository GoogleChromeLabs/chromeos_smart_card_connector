# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Turn off doxygen (it doesn't exist on Mac & Linux, and has an error on
# Windows).
export HAVE_DOXYGEN="false"
EXTRA_CONFIGURE_ARGS="\
  --disable-examples \
  --disable-sdltest \
  --${NACL_OPTION}-asm"
