# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Basic implemenation of FreeBSD pkg file format.

This is just enough to allow us to create archive files
that pkg will then be able to install.

See https://github.com/freebsd/pkg#pkgfmt for information
on this package format.
"""

import collections
import hashlib
import os
import shutil
import subprocess
import tarfile

from naclports import binary_package, package, paths, util
from naclports.error import Error

INSTALL_PREFIX = 'usr'

def WriteUCL(outfile, ucl_dict):
  """Write a dictionary to file in UCL format"""

  # See: https://github.com/vstakhov/libucl
  with open(outfile, 'w') as f:
    f.write('{\n')
    for key, value in ucl_dict.iteritems():
      if key == 'files':
        f.write('  "%s": \n  {\n' % key)
        for file_name, file_hash in value.iteritems():
          f.write('    "%s": "%s",\n' % (file_name, file_hash))
        f.write('  }\n')
      elif key == 'deps':
        f.write('  "%s": \n  {\n' % key)
        for dep_name, dep_dict in value.iteritems():
          f.write('    "%s": \n    {\n' % dep_name)
          for dep_dict_key, dep_dict_value in dep_dict.iteritems():
            f.write('        "%s": "%s",\n' % (dep_dict_key, dep_dict_value))
          f.write('    },\n')
        f.write('  }\n')
      else:
        f.write('  "%s": "%s",\n' % (key, value))
    f.write('}\n')


def ParseDir(payload_dir, file_dict, prefix):
  for filename in os.listdir(payload_dir):
    if not filename.startswith('+'):
      fullname = os.path.join(payload_dir, filename)
      if os.path.isdir(fullname):
        ParseDir(fullname, file_dict, prefix + filename + '/')
      elif os.path.islink(fullname):
        continue
      else:
        with open(fullname, 'rb') as f:
          # TODO(sbc): These extensions should probably be stripped out earlier
          # or never generated.
          basename, ext = os.path.splitext(filename)
          if ext in ('.nexe', '.pexe'):
            filename = basename
          file_dict[prefix + filename] = hashlib.sha256(f.read()).hexdigest()


def AddFilesInDir(content_dir, tar, prefix):
  for filename in os.listdir(content_dir):
    fullname = os.path.join(content_dir, filename)
    arcname = fullname.replace(prefix, '')

    # Resolve symbolic link by duplicating the target file
    # TODO(sbc): remove this once devenv (nacl_io) supports symlinks.
    if os.path.islink(fullname):
      link_target = os.path.realpath(fullname)
      if os.path.isabs(link_target):
        link_target = link_target.replace(binary_package.INSTALL_PREFIX, prefix
            + '/' + INSTALL_PREFIX)
      if not os.path.exists(link_target):
        raise Exception('Package contains dangling link: %s' % fullname)
      if os.path.isdir(link_target):
        continue
      os.remove(fullname)
      shutil.copyfile(link_target, fullname)

    if os.path.isdir(fullname):
      AddFilesInDir(fullname, tar, prefix)
    elif os.path.islink(fullname):
      info = tar.gettarinfo(fullname)
      info.name = arcname
      tar.addfile(info)
    else:
      # Rather convoluted way to add files to a tar archive that are
      # abolute (i.e. start with /).  pkg requires this, but python's
      # tar.py calls lstrip('/') on arcname in the tar.add() method.
      with open(fullname, 'r') as f:
        info = tar.gettarinfo(fileobj=f)
        info.name = arcname
        # TODO(sbc): These extensions should probably be stripped out earlier
        # or never generated.
        basename, ext = os.path.splitext(info.name)
        if ext in ('.nexe', '.pexe'):
          info.name = basename
        tar.addfile(info, fileobj=f)


DEFAULT_LOCATIONS = ('ports', 'ports/python_modules')


def AddPackageDep(dep_dict, dep):
  dep_dict['origin'] = dep

  if os.path.isdir(dep):
    dep_dict['version'] = package.Package(info_file =
                                          os.path.join(dep, 'pkg_info')).VERSION
    return

  for subdir in DEFAULT_LOCATIONS:
    pkg_info_file = os.path.join(paths.NACLPORTS_ROOT,
                                 subdir, dep, 'pkg_info')

    if os.path.exists(pkg_info_file):
      dep_dict['version'] = package.Package(info_file=pkg_info_file).VERSION
      return

  raise Error("Package not found: %s" % dep)


# These packages are are built-time only depednecies and we won't want
# encode them into the pkg file deps.
BUILD_ONLY_DEPS = [
  'glibc-compat',
  'libtar',
  'python-host',
  'gmp',
  'mpfr',
  'mpc',
]

def CreateDependencies(depends_dict, depends):
  for dep in depends:
    if dep in BUILD_ONLY_DEPS:
      continue
    dep_dict = collections.OrderedDict()
    AddPackageDep(dep_dict, dep)
    depends_dict[dep] = dep_dict


def CreatePkgFile(name, version, arch, payload_dir, outfile, depends):
  """Create an archive file in FreeBSD's pkg file format"""
  util.Log('Creating pkg package: %s' % outfile)
  manifest = collections.OrderedDict()
  manifest['name'] = name
  manifest['version'] =  version
  manifest['arch'] = 'nacl:0:%s' % arch

  # The following fields are required by 'pkg' but we don't have
  # meaningful values for them yet
  manifest['origin'] = name,
  manifest['comment'] = 'comment not available'
  manifest['desc'] = 'desc not available'
  manifest['maintainer'] = 'native-client-discuss@googlegroups.com'
  manifest['www'] = 'https://code.google.com/p/naclports/'
  manifest['prefix'] = INSTALL_PREFIX

  if depends:
    depends_dict = collections.OrderedDict()
    CreateDependencies(depends_dict, depends)
    manifest['deps'] = depends_dict

  temp_dir = os.path.splitext(outfile)[0] + '.tmp'
  if os.path.exists(temp_dir):
    shutil.rmtree(temp_dir)
  os.mkdir(temp_dir)

  content_dir = os.path.join(temp_dir, INSTALL_PREFIX)
  shutil.copytree(payload_dir, content_dir, symlinks=True)
  WriteUCL(os.path.join(temp_dir, '+COMPACT_MANIFEST'), manifest)
  file_dict = collections.OrderedDict()
  ParseDir(temp_dir, file_dict, '/')
  manifest['files'] = file_dict
  WriteUCL(os.path.join(temp_dir, '+MANIFEST'), manifest)

  with tarfile.open(outfile, 'w:bz2') as tar:
    for filename in os.listdir(temp_dir):
      if filename.startswith('+'):
        fullname = os.path.join(temp_dir, filename)
        tar.add(fullname, arcname=filename)

    for filename in os.listdir(temp_dir):
      if not filename.startswith('+'):
        fullname = os.path.join(temp_dir, filename)
        AddFilesInDir(fullname, tar, temp_dir)
  shutil.rmtree(temp_dir)
