#!/bin/bash
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Executable entry point for building nacl ports.
# This script must be run from the indivdual port directory (containing
# pkg_info and build.sh).  Most of the code and heavy lifting is in
# common.sh.

readonly SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE}")" ; pwd)

if [ ! -f ./pkg_info ]; then
  echo "No pkg_info found in current directory"
  exit 1
fi
source ./pkg_info

source ${SCRIPT_DIR}/common.sh

if [ -f ./build.sh ]; then
  source ./build.sh
fi

NaclportsMain "$@"
