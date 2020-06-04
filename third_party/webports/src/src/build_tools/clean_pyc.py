#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to cleanup stale .pyc files.

This script is run by gclient run-hooks (see DEPS)
"""

from __future__ import print_function

import argparse
import os
import sys


def main(args):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('directories', metavar='DIR', nargs='+',
                      help='directory to search for pyc files')
  args = parser.parse_args()
  for directory in args.directories:
    assert os.path.isdir(directory)
    for root, _, files in os.walk(directory):
      for filename in files:
        basename, ext = os.path.splitext(filename)
        if ext == '.pyc':
          py_file = os.path.join(root, basename + '.py')
          if not os.path.exists(py_file):
            pyc_file = os.path.join(root, filename)
            print('Removing stale pyc file: %s' % pyc_file)
            os.remove(pyc_file)


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
