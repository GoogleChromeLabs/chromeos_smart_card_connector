#!/usr/bin/env python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to synchronise the naclports mirror of upstream archives.

This script verifies that the URL for every package is mirrored on
Google Cloud Storage.  If it finds missing URLs it downloads them to
the local machine and then pushes them up using gsutil.

If any mirroring operations are required then the correct gsutil
credentials will be needed.
"""

from __future__ import print_function

import argparse
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACLPORTS_ROOT = os.path.dirname(SCRIPT_DIR)
sys.path.append(os.path.join(NACLPORTS_ROOT, 'lib'))

import naclports
import naclports.source_package

MIRROR_GS = 'gs://naclports/mirror'


def GsUpload(options, filename, url):
  """Upload a file to Google cloud storage using gsutil"""
  naclports.Log("Uploading to mirror: %s" % url)
  cmd = options.gsutil + ['cp', '-a', 'public-read', filename, url]
  if options.dry_run:
    naclports.Log(cmd)
  else:
    subprocess.check_call(cmd)


def GetMirrorListing(options, url):
  """Get filename listing for a Google cloud storage URL"""
  listing = subprocess.check_output(options.gsutil + ['ls', url])
  listing = listing.splitlines()
  listing = [os.path.basename(l) for l in listing]
  return listing


def CheckMirror(options, package, mirror_listing):
  """Check that is package has is archive mirrors on Google cloud storage"""
  naclports.LogVerbose('Checking %s' % package.NAME)
  basename = package.GetArchiveFilename()
  if not basename:
    return

  if basename in mirror_listing:
    # already mirrored
    return

  if options.check:
    naclports.Log('update_mirror: Archive missing from mirror: %s' % basename)
    sys.exit(1)

  # Download upstream URL
  package.Download(force_mirror=False)

  url = '%s/%s' % (MIRROR_GS, basename)
  GsUpload(options, package.DownloadLocation(), url)


def CheckPackages(options, source_packages, mirror_listing):
  count = 0
  for package in source_packages:
    CheckMirror(options, package, mirror_listing)
    count += 1

  if options.check:
    naclports.Log("Verfied mirroring for %d packages" % count)

  return 0


def main(args):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('-n', '--dry-run', action='store_true',
                      help="Don't actually upload anything")
  parser.add_argument('--check', action='store_true',
                      help='Verify that the mirror is up-to-date.')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='Enable verbose output.')
  options = parser.parse_args(args)
  naclports.SetVerbose(options.verbose)

  # gsutil.py should be in PATH since its part of depot_tools.
  options.gsutil = [sys.executable, naclports.util.FindInPath('gsutil.py')]

  listing = GetMirrorListing(options, MIRROR_GS)
  source_packages = naclports.source_package.SourcePackageIterator()

  return CheckPackages(options, source_packages, listing)


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except naclports.Error as e:
    sys.stderr.write('%s\n' % e)
    sys.exit(-1)
