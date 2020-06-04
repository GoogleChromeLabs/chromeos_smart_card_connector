# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ConfigureStep() {
  SetupCrossEnvironment
  local extra_args=""
  if [ "${TOOLCHAIN}" = pnacl ]; then
    extra_args="--cc=pnacl-clang"
  elif [ "${TOOLCHAIN}" = "clang-newlib" ]; then
    extra_args="--cc=${CC}"
  fi

  LogExecute ${SRC_DIR}/configure \
    --cross-prefix=${NACL_CROSS_PREFIX}- \
    --arch=${NACL_ARCH} \
    --target-os=linux \
    --enable-gpl \
    --enable-static \
    --enable-cross-compile \
    --disable-asm \
    --disable-inline-asm \
    --disable-programs \
    --disable-ssse3 \
    --disable-mmx \
    --disable-mmxext \
    --disable-amd3dnow \
    --disable-amd3dnowext \
    --disable-indevs \
    --disable-protocols \
    --disable-network \
    --enable-protocol=file \
    --enable-libmp3lame \
    --enable-libvorbis \
    --disable-demuxer=rtsp \
    --disable-demuxer=image2 \
    --prefix=${PREFIX} \
    ${extra_args}
}
