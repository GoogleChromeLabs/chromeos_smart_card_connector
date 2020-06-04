#!/usr/bin/env python
# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Find which packages are effected by a given change.

Accepts a list of changed files and outputs a list of effected
packages.  Outputs 'all' if any shared/non-package-specific
file if changed."""

import argparse
import fnmatch
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACLPORTS_ROOT = os.path.dirname(SCRIPT_DIR)
sys.path.append(os.path.join(NACLPORTS_ROOT, 'lib'))

import naclports
import naclports.source_package

def main(args):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('-v', '--verbose', action='store_true')
  parser.add_argument('--deps', action='store_true',
                      help='include dependencies of effected packages.')
  parser.add_argument('files', nargs='+', help='Changes files.')
  options = parser.parse_args(args)
  naclports.SetVerbose(options.verbose)

  if options.deps:
    package_filter = sys.stdin.read().split()
  else:
    package_filter = None

  effected_packages = find_effected_packages(options.files, options.deps,
      package_filter)
  print '\n'.join(effected_packages)
  return 0

# Normally when changins files outside of the 'ports' directory will
# trigger the rebuilding of all packages.  However certainly files are
# known to not effect the building on packages and those are listed here.
IGNORE_FILES = [
    'build_tools/find_effected_packages.py',
    'build_tools/partition*.txt',
    '*/test_*.py',
]

def find_effected_packages(files, include_deps, package_filter):
  packages = []
  to_resolve = []

  def AddPackage(package):
    if package_filter and package.NAME not in package_filter:
      naclports.LogVerbose('Filtered out package: %s' % package.NAME)
      return
    if package.NAME not in packages:
      if include_deps:
        for dep in package.TransitiveDependencies():
          if dep.NAME not in packages:
            packages.append(dep.NAME)
      packages.append(package.NAME)
      to_resolve.append(package)

  for filename in files:
    parts = filename.split(os.path.sep)
    if parts[0] != 'ports':
      naclports.LogVerbose('effected file outside of ports tree: %s' % filename)
      if any(fnmatch.fnmatch(filename, ignore) for ignore in IGNORE_FILES):
        continue
      return ['all']

    package_name = parts[1]
    pkg = naclports.source_package.CreatePackage(package_name)
    AddPackage(pkg)

  while to_resolve:
    pkg = to_resolve.pop()
    for r in pkg.ReverseDependencies():
      AddPackage(r)

  if package_filter:
    packages = [p for p in packages if p in package_filter]
  return packages


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except naclports.Error as e:
    sys.stderr.write('%s\n' % e)
    sys.exit(-1)
