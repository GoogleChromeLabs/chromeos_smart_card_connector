#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Large tests of the devenv app."""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(os.path.dirname(SCRIPT_DIR))
sys.path.append(SRC_DIR)
TOOLCHAIN = os.environ.get('TOOLCHAIN', 'newlib')
DEVENV_OUT_DIR = os.path.join(SRC_DIR, 'out', 'publish', 'devenv', TOOLCHAIN)

import chrome_test


app = os.path.join(DEVENV_OUT_DIR, 'app')
test_dir = os.path.join(SCRIPT_DIR, 'tests')

chrome_test.Main([
    '-C', test_dir,
    '-p', 'TOOLCHAIN=' + TOOLCHAIN,
    '-t', '180',
    '--enable-nacl',
    '--load-extension', app,
    'devenv_large_test.html'] + sys.argv[1:])
