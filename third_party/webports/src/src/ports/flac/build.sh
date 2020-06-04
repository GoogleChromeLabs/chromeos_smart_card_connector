# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXTRA_CONFIGURE_ARGS+=" --disable-oggtest"
EXTRA_CONFIGURE_ARGS+=" --disable-xmms-plugin"
EXTRA_CONFIGURE_ARGS+=" --without-metaflac-test-files"

EnableGlibcCompat

InstallStep() {
  # assumes pwd has makefile
  LogExecute make install-exec DESTDIR=${DESTDIR}
  ChangeDir include
  LogExecute make install DESTDIR=${DESTDIR}
}
