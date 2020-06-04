#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests of gdb and debugger."""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(SCRIPT_DIR, '../..'))
SRC_DIR = os.path.dirname(os.path.dirname(SCRIPT_DIR))
LIBC = os.environ.get('TOOLCHAIN', 'newlib')
GDB_OUT_DIR = os.path.join(SRC_DIR, 'out/publish/gdb', LIBC)

import chrome_test


gdb_app = os.path.join(GDB_OUT_DIR, 'gdb_app')
debug_extension = os.path.join(GDB_OUT_DIR, 'debug_extension')
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
      '--load-extension', debug_extension,
      'debugger_test.html'] + sys.argv[1:])
