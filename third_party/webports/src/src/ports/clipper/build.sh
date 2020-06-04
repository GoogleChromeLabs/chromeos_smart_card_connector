# Copyright 2015 The Native Client Authors. All rights reserved.
# # Use of this source code is governed by a BSD-style license that can be
# # found in the LICENSE file.

if [ "${NACL_SHARED}" != "1" ]; then
  EXTRA_CMAKE_ARGS="-DBUILD_SHARED_LIBS=OFF"
fi
