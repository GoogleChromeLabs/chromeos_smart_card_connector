#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test of the extension testing system itself."""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, SRC_DIR)

import chrome_test

if __name__ == '__main__':
  chrome_test.Main(
      ['-C', os.path.join(SCRIPT_DIR, 'tests'),
       '--load-extension', os.path.join(SCRIPT_DIR, 'pinger'),
       'plumbing_test.html'] + sys.argv[1:])
