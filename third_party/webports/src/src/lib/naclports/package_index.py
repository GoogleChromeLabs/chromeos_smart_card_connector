# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from naclports import configuration, binary_package, package, util, paths
from naclports import pkg_info, error

DEFAULT_INDEX = os.path.join(paths.NACLPORTS_ROOT, 'lib', 'prebuilt.txt')
EXTRA_KEYS = package.EXTRA_KEYS + ['BIN_URL', 'BIN_SIZE', 'BIN_SHA1']
PREBUILT_ROOT = os.path.join(paths.PACKAGES_ROOT, 'prebuilt')


def ExtractPkgInfo(filename):
  """Return the pkg_info contents from a binary package."""
  pkg = binary_package.BinaryPackage(filename)
  return pkg.GetPkgInfo()


def WriteIndex(index_filename, binaries):
  """Create a package index file from set of binaries on disk.

  Args:
    index_filename: The name of the file to write create.
    binaries: List of (filename, url) pairs containing packages to include.

  Returns:
    A PackageIndex object based on the contents of the newly written file.
  """
  # Write index to a temporary file and then rename it, to avoid
  # leaving a partial index file on disk.
  tmp_name = index_filename + '.tmp'
  with open(tmp_name, 'w') as output_file:
    for i, (filename, url) in enumerate(binaries):
      sha1 = util.HashFile(filename)
      if i != 0:
        output_file.write('\n')
      output_file.write(ExtractPkgInfo(filename))
      output_file.write('BIN_URL=%s\n' % url)
      output_file.write('BIN_SIZE=%s\n' % os.path.getsize(filename))
      output_file.write('BIN_SHA1=%s\n' % sha1)

  os.rename(tmp_name, index_filename)

  return IndexFromFile(index_filename)


def IndexFromFile(filename):
  with open(filename) as f:
    contents = f.read()
  return PackageIndex(filename, contents)


def GetCurrentIndex():
  if not os.path.exists(DEFAULT_INDEX):
    return PackageIndex('', '')
  return IndexFromFile(DEFAULT_INDEX)


class PackageIndex(object):
  """In memory representation of a package index file.

  This class is used to read a package index of disk and stores
  it in memory as dictionary keys on package name + configuration.
  """
  valid_keys = pkg_info.VALID_KEYS + EXTRA_KEYS
  required_keys = pkg_info.REQUIRED_KEYS + EXTRA_KEYS

  def __init__(self, filename, index_data):
    self.filename = filename
    self.packages = {}
    self.ParseIndex(index_data)

  def Contains(self, package_name, config):
    """Returns True if the index contains the given package in the given
    configuration, False otherwise."""
    return (package_name, config) in self.packages

  def Installable(self, package_name, config):
    """Returns True if the index contains the given package and it is
    installable in the currently configured SDK."""
    info = self.packages.get((package_name, config))
    if not info:
      return False
    version = util.GetSDKVersion()
    if info['BUILD_SDK_VERSION'] != version:
      util.LogVerbose('Prebuilt package was built with different SDK version: '
                      '%s vs %s' % (info['BUILD_SDK_VERSION'], version))
      return False
    return True

  def Download(self, package_name, config):
    if not os.path.exists(PREBUILT_ROOT):
      util.Makedirs(PREBUILT_ROOT)
    info = self.packages[(package_name, config)]
    filename = os.path.join(PREBUILT_ROOT, os.path.basename(info['BIN_URL']))
    if os.path.exists(filename):
      try:
        util.VerifyHash(filename, info['BIN_SHA1'])
        return filename
      except util.HashVerificationError:
        pass
    util.Log('Downloading prebuilt binary ...')
    util.DownloadFile(filename, info['BIN_URL'])
    util.VerifyHash(filename, info['BIN_SHA1'])
    return filename

  def ParseIndex(self, index_data):
    if not index_data:
      return

    for info_files in index_data.split('\n\n'):
      info = pkg_info.ParsePkgInfo(info_files, self.filename, self.valid_keys,
                                   self.required_keys)
      debug = info['BUILD_CONFIG'] == 'debug'
      config = configuration.Configuration(info['BUILD_ARCH'],
                                           info['BUILD_TOOLCHAIN'], debug)
      key = (info['NAME'], config)
      if key in self.packages:
        error.Error('package index contains duplicate: %s' % str(key))
      self.packages[key] = info
