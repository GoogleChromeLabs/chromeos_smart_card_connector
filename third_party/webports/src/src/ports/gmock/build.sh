# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

InstallStep() {
  LogExecute tar -C ${SRC_DIR}/include -cf - gmock | \
    tar -C ${DESTDIR_INCLUDE} -xpf -
  LogExecute install -m 644 lib/.libs/libgmock.a ${DESTDIR_LIB}/
}
