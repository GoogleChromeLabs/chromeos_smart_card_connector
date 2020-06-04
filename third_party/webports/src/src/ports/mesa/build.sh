# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Mesa uses autoconf but not automake and its handwritten Makefiles do not
# support out-of-tree building
BUILD_DIR=${SRC_DIR}

EXTRA_CONFIGURE_ARGS="\
  --enable-static \
  --disable-gl-osmesa \
  --with-driver=osmesa \
  --disable-asm \
  --disable-glut \
  --disable-gallium \
  --disable-egl \
  --disable-glw"

ConfigureStep() {
  export X11_INCLUDES=
  DefaultConfigureStep
}

BuildStep() {
  export AR=${NACLAR}
  DefaultBuildStep
}
