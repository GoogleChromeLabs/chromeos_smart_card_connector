#!/bin/bash
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SCRIPT_DIR=$(dirname "$BASH_SOURCE")

NACL_ENV_IMPORT=1
. "$SCRIPT_DIR/nacl-env.sh"
NaClEnvExport

exec make DESTDIR=$NACL_PREFIX "$@"
