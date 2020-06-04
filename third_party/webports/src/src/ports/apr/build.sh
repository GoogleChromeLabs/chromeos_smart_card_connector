# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EnableGlibcCompat

if [ "${NACL_SHARED}" != "1" ]; then
  EXTRA_CONFIGURE_ARGS="--disable-dso"
fi
