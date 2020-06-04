#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of gdb app component."""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(SCRIPT_DIR, '../..'))
SRC_DIR = os.path.dirname(os.path.dirname(SCRIPT_DIR))
TOOLCHAIN = os.environ.get('TOOLCHAIN', 'newlib')
GDB_OUT_DIR = os.path.join(SRC_DIR, 'out/publish/gdb', TOOLCHAIN)

import chrome_test


gdb_app = os.path.join(GDB_OUT_DIR, 'gdb_app')
test_dir = os.path.join(SCRIPT_DIR, 'tests')
test_out_dir = os.path.join(GDB_OUT_DIR, 'tests')

if __name__ == '__main__':
  chrome_test.Main([
      '-C', test_dir,
      '-C', test_out_dir,
      '-t', '60',
      '--enable-nacl',
      '--enable-nacl-debug',
      '--load-and-launch-app', gdb_app,
      'gdb_test.html'] + sys.argv[1:])
