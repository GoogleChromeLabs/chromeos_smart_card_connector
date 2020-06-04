# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"

NACLPORTS_LIBS+="-pthread -lnacl_io -l${NACL_CXX_LIB}"

EnableGlibcCompat
