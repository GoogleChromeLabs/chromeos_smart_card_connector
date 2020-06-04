# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES=avrdude${NACL_EXEEXT}
export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"

EnableGlibcCompat
