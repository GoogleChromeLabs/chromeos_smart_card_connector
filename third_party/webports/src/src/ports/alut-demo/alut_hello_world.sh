#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

source ../../build_tools/common.sh

# Always 'make clean' because there could be artifacts from a previous build
# in this sub-folder, 'make clean' from the top-level only clears 'out'.
export CFLAGS=${NACLPORTS_CFLAGS}
export LDFLAGS=${NACLPORTS_LDFLAGS}
make clean
make
