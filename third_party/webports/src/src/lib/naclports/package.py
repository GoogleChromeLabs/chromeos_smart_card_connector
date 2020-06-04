# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from naclports.util import Log, Warn, LogVerbose
from naclports.error import Error
from naclports import configuration, pkg_info, util

EXTRA_KEYS = ['BUILD_CONFIG', 'BUILD_ARCH', 'BUILD_TOOLCHAIN',
              'BUILD_SDK_VERSION', 'BUILD_NACLPORTS_REVISION']


def RemoveEmptyDirs(dirname):
  """Recursively remove a directoy and its parents if they are empty."""
  while not os.listdir(dirname):
    os.rmdir(dirname)
    dirname = os.path.dirname(dirname)


def RemoveFile(filename):
  os.remove(filename)
  RemoveEmptyDirs(os.path.dirname(filename))


class Package(object):
  extra_keys = []
  list_props = (
    'DEPENDS',
    'CONFLICTS',
    'DISABLED_ARCH',
    'DISABLED_LIBC',
    'DISABLED_TOOLCHAIN'
  )

  def __init__(self, info_file=None):
    self.info = info_file
    if self.info:
      with open(self.info) as f:
        self.ParseInfo(f.read())

  def ParseInfo(self, info_string):
    valid_keys = pkg_info.VALID_KEYS + self.extra_keys
    required_keys = pkg_info.REQUIRED_KEYS + self.extra_keys

    for key in valid_keys:
      if key in self.list_props:
        setattr(self, key, [])
      else:
        setattr(self, key, None)

    # Parse pkg_info file
    info = pkg_info.ParsePkgInfo(info_string, self.info, valid_keys,
                                 required_keys)

    # Set attributres based on pkg_info setttings.
    for key, value in info.items():
      setattr(self, key, value)

    self.Validate()

  def Validate(self):
    for libc in self.DISABLED_LIBC:
      if libc not in configuration.VALID_LIBC:
        raise Error('%s: invalid libc: %s' % (self.info, libc))

    for toolchain in self.DISABLED_TOOLCHAIN:
      if '/' in toolchain:
        toolchain, arch = toolchain.split('/')
        if arch not in util.arch_to_pkgarch:
          raise Error('%s: invalid architecture: %s' % (self.info, arch))
      if toolchain not in configuration.VALID_TOOLCHAINS:
        raise Error('%s: invalid toolchain: %s' % (self.info, toolchain))

    for arch in self.DISABLED_ARCH:
      if arch not in util.arch_to_pkgarch:
        raise Error('%s: invalid architecture: %s' % (self.info, arch))

    if '_' in self.NAME:
      raise Error('%s: package NAME cannot contain underscores' % self.info)

    if self.NAME != self.NAME.lower():
      raise Error('%s: package NAME cannot contain uppercase characters' %
                  self.info)

    if '_' in self.VERSION:
      raise Error('%s: package VERSION cannot contain underscores' % self.info)

    if self.DISABLED_ARCH and self.ARCH is not None:
      raise Error('%s: contains both ARCH and DISABLED_ARCH' % self.info)

    if self.DISABLED_LIBC and self.LIBC is not None:
      raise Error('%s: contains both LIBC and DISABLED_LIBC' % self.info)

  def __cmp__(self, other):
    return cmp((self.NAME, self.VERSION, self.config),
               (other.NAME, other.VERSION, other.config))

  def __hash__(self):
    return hash((self.NAME, self.VERSION, self.config))

  def __str__(self):
    return '<Package %s %s %s>' % (self.NAME, self.VERSION, self.config)

  def InfoString(self):
    return "'%s' [%s]" % (self.NAME, self.config)

  def LogStatus(self, message, suffix=''):
    util.LogHeading(message, " '%s' [%s] %s" %
                    (util.Color(self.NAME, 'yellow'),
                     util.Color(self.config, 'blue'), suffix))

  def CheckDeps(self, valid_packages):
    for package in self.DEPENDS:
      if package not in valid_packages:
        Log('%s: Invalid dependency: %s' % (self.info, package))
        return False

    for package in self.CONFLICTS:
      if package not in valid_packages:
        Log('%s: Invalid conflict: %s' % (self.info, package))
        return False

    return True

  def IsAnyVersionInstalled(self):
    return util.IsInstalled(self.NAME, self.config)

  def GetInstallStamp(self):
    """Returns the name of install stamp for this package."""
    return util.GetInstallStamp(self.NAME, self.config)

  def GetListFile(self):
    """Returns the name of the installed file list for this package."""
    return util.GetListFile(self.NAME, self.config)


class InstalledPackage(Package):
  extra_keys = EXTRA_KEYS

  def __init__(self, info_file):
    super(InstalledPackage, self).__init__(info_file)
    self.config = configuration.Configuration(self.BUILD_ARCH,
                                              self.BUILD_TOOLCHAIN,
                                              self.BUILD_CONFIG == 'debug')

  def Uninstall(self):
    self.LogStatus('Uninstalling')
    self.DoUninstall()

  def Files(self):
    """Yields the list of files currently installed by this package."""
    file_list = self.GetListFile()
    if not os.path.exists(file_list):
      return
    with open(self.GetListFile()) as f:
      for line in f:
        yield line.strip()

  def DoUninstall(self):
    with util.InstallLock(self.config):
      RemoveFile(self.GetInstallStamp())

      root = util.GetInstallRoot(self.config)
      for filename in self.Files():
        fullname = os.path.join(root, filename)
        if not os.path.lexists(fullname):
          Warn('File not found while uninstalling: %s' % fullname)
          continue
        LogVerbose('uninstall: %s' % filename)
        RemoveFile(fullname)

      if os.path.exists(self.GetListFile()):
        RemoveFile(self.GetListFile())


def InstalledPackageIterator(config):
  stamp_root = util.GetInstallStampRoot(config)
  if not os.path.exists(stamp_root):
    return

  for filename in os.listdir(stamp_root):
    if os.path.splitext(filename)[1] != '.info':
      continue
    yield InstalledPackage(os.path.join(stamp_root, filename))


def CreateInstalledPackage(package_name, config=None):
  stamp_root = util.GetInstallStampRoot(config)
  info_file = os.path.join(stamp_root, package_name + '.info')
  if not os.path.exists(info_file):
    raise Error('package not installed: %s [%s]' % (package_name, config))
  return InstalledPackage(info_file)
