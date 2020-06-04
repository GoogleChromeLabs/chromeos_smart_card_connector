#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Scan online binary packages to produce new package index.

This script is indended to be run periodically and the
results checked into source control.
"""

from __future__ import print_function

import argparse
import base64
import collections
import hashlib
import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(os.path.dirname(SCRIPT_DIR), 'lib'))

import naclports
import naclports.package
import naclports.package_index

from naclports.util import Log, LogVerbose


def FormatSize(num_bytes):
  """Create a human readable string from a byte count."""
  for x in ('bytes', 'KB', 'MB', 'GB', 'TB'):
    if num_bytes < 1024.0:
      return "%3.1f %s" % (num_bytes, x)
    num_bytes /= 1024.0


class FileInfo(object):

  def __init__(self, name, url, gsurl, size=0, md5=''):
    self.name = name
    self.url = url
    self.gsurl = gsurl
    self.size = size
    self.md5 = md5

  def __repr__(self):
    return '<FileInfo %s [%s]>' % (self.name, self.size)


def ParseGsUtilOutput(output):
  """Parse the output of gsutil -L.

  Returns:
     List of FileInfo objects.
  """
  # gsutil stat outputs the name of each file starting in column zero followed
  # by zero or more fields indended with tab characters.
  # gs://naclports/builds/pepper_44/..../agg-demo_0.1_x86-64_newlib.tar.bz2:
  #  Creation time:    Wed, 13 May 2015 18:52:05 GMT
  #  Content-Length:   342
  #  Content-Type:     application/x-tar
  #  Hash (crc32c):    sf+mJQ==
  #  Hash (md5):       tXtj9ASElmzyncWl0k/PvA==
  #  ETag:             CIjv79uxv8UCEAE=
  #  Generation:       1431543125637000
  #  Metageneration:   1
  #
  # Note that some fields contiue onto more than one line.
  # The final line in the file starts with TOTAL

  result = []
  info = None
  for line in output.splitlines():
    if line[0] != '\t':
      if info is not None:
        assert info.size
        assert info.md5
        result.append(info)
      # Handle final line
      if line.startswith("TOTAL"):
        continue
      gsurl = line.strip()[:-1]
      filename = gsurl[len('gs://'):]
      url = naclports.GS_URL + filename
      info = FileInfo(name=line.strip(), url=url, gsurl=gsurl)
    else:
      line = line.strip()
      prop_name, prop_value = line.split(':', 1)
      if prop_name == 'Content-Length':
        info.size = int(prop_value)
      elif prop_name == 'Hash (md5)':
        info.md5 = base64.b64decode(prop_value).encode('hex')

  result.append(info)
  return result


def GetHash(filename):
  with open(filename) as f:
    return hashlib.md5(f.read()).hexdigest()


def CheckHash(filename, md5sum):
  """Return True is filename has the given md5sum, False otherwise."""
  return md5sum == GetHash(filename)


def DownloadFiles(files, check_hashes=True, parallel=False):
  """Download one of more files to the local disk.

  Args:
    files: List of FileInfo objects to download.
    check_hashes: When False assume local files have the correct
    hash otherwise always check the hashes match the onces in the
    FileInfo ojects.

  Returns:
    List of (filename, url) tuples.
  """
  files_to_download = []
  filenames = []
  download_dir = naclports.package_index.PREBUILT_ROOT
  if not os.path.exists(download_dir):
    os.makedirs(download_dir)

  for file_info in files:
    basename = os.path.basename(file_info.url)
    file_info.name = os.path.join(download_dir, basename)
    filenames.append((file_info.name, file_info.url))
    if os.path.exists(file_info.name):
      if not check_hashes or CheckHash(file_info.name, file_info.md5):
        Log('Up-to-date: %s' % file_info.name)
        continue
    files_to_download.append(file_info)

  def Check(file_info):
    if check_hashes and not CheckHash(file_info.name, file_info.md5):
      raise naclports.Error(
          'Checksum failed: %s\nExpected=%s\nActual=%s' %
          (file_info.name, file_info.md5, GetHash(file_info.name)))

  if not files_to_download:
    Log('All files up-to-date')
  else:
    total_size = sum(f.size for f in files_to_download)
    Log('Need to download %d/%d files [%s]' %
        (len(files_to_download), len(files), FormatSize(total_size)))

    gsutil = FindGsutil()
    if parallel:
      remaining_files = files_to_download
      num_files = 20
      while remaining_files:
        files = remaining_files[:num_files]
        remaining_files = remaining_files[num_files:]
        cmd = gsutil + ['-m', 'cp'] + [f.gsurl for f in files] + [download_dir]
        LogVerbose(cmd)
        subprocess.check_call(cmd)
        for file_info in files:
          Check(file_info)
    else:
      for file_info in files_to_download:
        naclports.DownloadFile(file_info.name, file_info.url)
        Check(file_info)

  return filenames


def FindGsutil():
  # Ideally we would use the gsutil that comes with depot_tools since users
  # are much more likely to have that in thier PATH.  However I found this
  # depot_tools version to be a lot slower.
  # return [sys.executable, naclports.util.FindInPath('gsutil.py')]
  return ['gsutil']


def main(args):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('revision', metavar='REVISION',
                      help='naclports revision to to scan for.')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='Output extra information.')
  parser.add_argument('-p', '--parallel', action='store_true',
                      help='Download packages in parallel.')
  parser.add_argument('-l', '--cache-listing', action='store_true',
                      help='Cached output of gsutil -L (for testing).')
  parser.add_argument('--skip-md5', action='store_true',
                      help='Assume on-disk files are up-to-date (for testing).')
  args = parser.parse_args(args)
  if args.verbose:
    naclports.SetVerbose(True)

  sdk_version = naclports.util.GetSDKVersion()
  Log('Scanning packages built for pepper_%s at revsion %s' %
      (sdk_version, args.revision))
  base_path = '%s/builds/pepper_%s/%s/packages' % (naclports.GS_BUCKET,
                                                   sdk_version, args.revision)
  gs_url = 'gs://' + base_path + '/*'
  listing_file = os.path.join(naclports.NACLPORTS_ROOT, 'lib', 'listing.txt')

  if args.cache_listing and os.path.exists(listing_file):
    Log('Using pre-cached gs listing: %s' % listing_file)
    with open(listing_file) as f:
      listing = f.read()
  else:
    Log('Searching for packages at: %s' % gs_url)
    cmd = FindGsutil() + ['stat', gs_url]
    LogVerbose('Running: %s' % str(cmd))
    try:
      listing = subprocess.check_output(cmd)
    except subprocess.CalledProcessError as e:
      raise naclports.Error("Command '%s' failed: %s" % (cmd, e))
    if args.cache_listing:
      with open(listing_file, 'w') as f:
        f.write(listing)

  all_files = ParseGsUtilOutput(listing)

  Log('Found %d packages [%s]' % (len(all_files),
                                  FormatSize(sum(f.size for f in all_files))))

  binaries = DownloadFiles(all_files, not args.skip_md5, args.parallel)
  index_file = os.path.join(naclports.NACLPORTS_ROOT, 'lib', 'prebuilt.txt')
  Log('Generating %s' % index_file)
  naclports.package_index.WriteIndex(index_file, binaries)
  Log('Done')
  return 0


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except naclports.Error as e:
    sys.stderr.write('%s\n' % e)
    sys.exit(-1)
