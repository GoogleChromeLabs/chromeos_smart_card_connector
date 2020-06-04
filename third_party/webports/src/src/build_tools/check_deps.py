#!/usr/bin/env python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tool which checks the dependencies of all packages.
"""

from __future__ import print_function

import argparse
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(os.path.dirname(SCRIPT_DIR), 'lib'))

import naclports
import naclports.source_package


def main(args):
  parser = argparse.ArgumentParser()
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='Output extra information.')
  options = parser.parse_args(args)
  if options.verbose:
    naclports.SetVerbose(True)
  count = 0

  package_names = [os.path.basename(p.root)
                   for p in naclports.source_package.SourcePackageIterator()]

  for package in naclports.source_package.SourcePackageIterator():
    if not package.CheckDeps(package_names):
      return 1
    count += 1
  naclports.Log("Verfied dependencies for %d packages" % count)
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
