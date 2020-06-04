# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import posixpath
import shutil
import stat
import tarfile

from naclports import configuration, package, util, error

PAYLOAD_DIR = 'payload'
INSTALL_PREFIX = '/naclports-dummydir'

def MakeDirIfNeeded(filename):
  dirname = os.path.dirname(filename)
  if not os.path.isdir(dirname):
    util.Makedirs(dirname)


def FilterOutExecutables(filenames, root):
  """Filter out ELF binaries in the bin directory.

  We don't want NaCl exectuables installed in the toolchain's bin directory
  since we add this to the PATH during the build process, and NaCl executables
  can't be run on the host system (not without sel_ldr anyway).
  """
  rtn = []
  for name in filenames:
    full_name = os.path.join(root, name)
    if os.path.split(name)[0] == 'bin':
      if not os.path.splitext(name)[1] and util.IsElfFile(full_name):
        continue
    rtn.append(name)

  return rtn


def InstallFile(filename, old_root, new_root):
  """Install a single file by moving it into a new location.

  Args:
    filename: Relative name of file to install.
    old_root: The current location of the file.
    new_root: The new desired root for the file.
  """
  oldname = os.path.join(old_root, filename)

  util.LogVerbose('install: %s' % filename)

  newname = os.path.join(new_root, filename)
  dirname = os.path.dirname(newname)
  if not os.path.isdir(dirname):
    util.Makedirs(dirname)
  os.rename(oldname, newname)

  # When install binaries ELF files into the toolchain direcoties, remove
  # the X bit so that they do not found when searching the PATH.
  if util.IsElfFile(newname) or util.IsPexeFile(newname):
    mode = os.stat(newname).st_mode
    mode = mode & ~(stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    os.chmod(newname, mode)



def RelocateFile(filename, dest):
  """Perform in-place mutations on file contents to handle new location.

  There are a few file types that have absolute pathnames embedded
  and need to be modified in some way when being installed to
  a particular location. For most file types this method does nothing.
  """
  # Only relocate certain file types.
  modify = False

  # boost build scripts
  # TODO(sbc): move this to the boost package metadata
  if filename.startswith('build-1'):
    modify = True
  # pkg_config (.pc) files
  if filename.startswith('lib/pkgconfig'):
    modify = True
  if filename.startswith('share/pkgconfig'):
    modify = True
  # <foo>-config scripts that live in usr/bin
  if filename.startswith('bin') and filename.endswith('-config'):
    modify = True
  # libtool's .la files which can contain absolute paths to
  # dependencies.
  if filename.endswith('.la'):
    modify = True
  # headers can sometimes also contain absolute paths.
  if filename.startswith('include/') and filename.endswith('.h'):
    modify = True

  filename = os.path.join(dest, filename)

  if modify:
    with open(filename) as f:
      data = f.read()
    mode = os.stat(filename).st_mode
    os.chmod(filename, 0777)
    with open(filename, 'r+') as f:
      f.write(data.replace(INSTALL_PREFIX, dest))
    os.chmod(filename, mode)


class BinaryPackage(package.Package):
  """Representation of binary package packge file.

  This class is initialised with the filename of a binary package
  and its attributes are set according the file name and contents.

  Operations such as installation can be performed on the package.
  """
  extra_keys = package.EXTRA_KEYS

  def __init__(self, filename):
    super(BinaryPackage, self).__init__()
    self.filename = filename
    self.info = filename
    self.VerifyArchiveFormat()

    info = self.GetPkgInfo()
    self.ParseInfo(info)
    self.config = configuration.Configuration(self.BUILD_ARCH,
                                              self.BUILD_TOOLCHAIN,
                                              self.BUILD_CONFIG == 'debug')

  def VerifyArchiveFormat(self):
    if not os.path.exists(self.filename):
      raise error.Error('package archive not found: %s' % self.filename)
    basename, extension = os.path.splitext(os.path.basename(self.filename))
    basename = os.path.splitext(basename)[0]
    if extension != '.bz2':
      raise error.Error('invalid file extension: %s' % extension)

    try:
      with tarfile.open(self.filename) as tar:
        if './pkg_info' not in tar.getnames():
          raise error.PkgFormatError('package does not contain pkg_info file')
    except tarfile.TarError as e:
      raise error.PkgFormatError(e)

  def IsInstallable(self):
    """Determine if a binary package can be installed in the
    currently configured SDK.

    Currently only packages built with the same SDK major version
    are installable.
    """
    return self.BUILD_SDK_VERSION == util.GetSDKVersion()

  def GetPkgInfo(self):
    """Extract the contents of the pkg_info file from the binary package."""
    with tarfile.open(self.filename) as tar:
      return tar.extractfile('./pkg_info').read()

  def Install(self, force):
    """Install binary package into toolchain directory."""
    with util.InstallLock(self.config):
      self._Install(force)

  def _Install(self, force):
    if self.TOOLCHAIN_INSTALL != '0':
        self._InstallFiles(force)
    self.WriteStamp()

  def _InstallFiles(self, force):
    dest = util.GetInstallRoot(self.config)
    dest_tmp = os.path.join(dest, 'install_tmp')
    if os.path.exists(dest_tmp):
      shutil.rmtree(dest_tmp)

    if self.IsAnyVersionInstalled():
      raise error.Error('package already installed: %s' % self.InfoString())

    self.LogStatus('Installing')
    util.LogVerbose('installing from: %s' % self.filename)
    util.Makedirs(dest_tmp)

    names = []
    try:
      with tarfile.open(self.filename) as tar:
        for info in tar:
          if info.isdir():
            continue
          name = posixpath.normpath(info.name)
          if name == 'pkg_info':
            continue
          if not name.startswith(PAYLOAD_DIR + '/'):
            raise error.PkgFormatError('invalid file in package: %s' % name)

          name = name[len(PAYLOAD_DIR) + 1:]
          names.append(name)

        if not force:
          for name in names:
            full_name = os.path.join(dest, name)
            if os.path.exists(full_name) :
              raise error.Error('file already exists: %s' % full_name)

        tar.extractall(dest_tmp)
        payload_tree = os.path.join(dest_tmp, PAYLOAD_DIR)

        names = FilterOutExecutables(names, payload_tree)

        for name in names:
          InstallFile(name, payload_tree, dest)
    finally:
      shutil.rmtree(dest_tmp)

    for name in names:
      RelocateFile(name, dest)

    self.WriteFileList(names)

  def WriteStamp(self):
    """Write stamp file containing pkg_info."""
    filename = util.GetInstallStamp(self.NAME, self.config)
    MakeDirIfNeeded(filename)
    util.LogVerbose('stamp: %s' % filename)
    pkg_info = self.GetPkgInfo()
    with open(filename, 'w') as f:
      f.write(pkg_info)

  def WriteFileList(self, file_names):
    """Write the file list for this package."""
    filename = self.GetListFile()
    MakeDirIfNeeded(filename)
    with open(filename, 'w') as f:
      for name in file_names:
        f.write(name + '\n')
