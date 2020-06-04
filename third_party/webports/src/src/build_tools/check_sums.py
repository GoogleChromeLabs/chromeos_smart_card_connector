#!/usr/bin/env python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tool which checks the sha1 sums of all packages.
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
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='Output extra information.')
  options = parser.parse_args(args)
  if options.verbose:
    naclports.SetVerbose(True)
  count = 0

  for package in naclports.source_package.SourcePackageIterator():
    package.Download()
    if not package.Verify():
      return 1

    count += 1

  naclports.Log("Verfied checksums for %d packages" % count)
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
