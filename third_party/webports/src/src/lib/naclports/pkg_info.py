# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import shlex
import string

from naclports.error import PkgFormatError

VALID_KEYS = ['NAME', 'VERSION', 'URL', 'ARCHIVE_ROOT', 'LICENSE', 'DEPENDS',
              'MIN_SDK_VERSION', 'LIBC', 'DISABLED_LIBC', 'ARCH', 'CONFLICTS',
              'DISABLED_ARCH', 'URL_FILENAME', 'BUILD_OS', 'SHA1', 'DISABLED',
              'DISABLED_TOOLCHAIN', 'TOOLCHAIN_INSTALL', 'PATCH_NAME']
REQUIRED_KEYS = ['NAME', 'VERSION']


def ParsePkgInfo(contents, filename, valid_keys=None, required_keys=None):
  """Parse a string contains the contents of a pkg_info file.

  Args:
    contents: pkg_info contents as a string.
    filename: name of file to use in error messages.
    valid_keys: list of keys that are valid in the file.
    required_keys: list of keys that are required in the file.

  Returns:
    A dictionary of the key, value pairs contained in the pkg_info file.

  Raises:
    PkgFormatError: if file is malformed, contains invalid keys, or does not
      contain all required keys.
  """
  rtn = {}
  if valid_keys is None:
    valid_keys = VALID_KEYS
  if required_keys is None:
    required_keys = REQUIRED_KEYS

  def ParsePkgInfoLine(line, line_no):
    if '=' not in line:
      raise PkgFormatError('Invalid info line %s:%d' % (filename, line_no))
    key, value = line.split('=', 1)
    key = key.strip()
    if key not in valid_keys:
      raise PkgFormatError("Invalid key '%s' in info file %s:%d" %
                           (key, filename, line_no))
    value = value.strip()
    if value[0] == '(':
      if value[-1] != ')':
        raise PkgFormatError('Error parsing %s:%d: %s (%s)' %
                             (filename, line_no, key, value))
      value = value[1:-1].split()
    else:
      value = shlex.split(value)[0]
    return (key, value)

  def ExpandVars(value, substitutions):
    if type(value) == str:
      return string.Template(value).substitute(substitutions)
    else:
      return [string.Template(v).substitute(substitutions) for v in value]

  for i, line in enumerate(contents.splitlines()):
    if not line or line[0] == '#':
      continue
    key, raw_value = ParsePkgInfoLine(line, i + 1)
    if key in rtn:
      raise PkgFormatError('Error parsing %s:%d: duplicate key (%s)' %
                           (filename, i + 1, key))
    rtn[key] = ExpandVars(raw_value, rtn)

  for required_key in required_keys:
    if required_key not in rtn:
      raise PkgFormatError("Required key '%s' missing from info file: '%s'" %
                           (required_key, filename))

  return rtn


def ParsePkgInfoFile(filename, valid_keys=None, required_keys=None):
  """Parse pkg_info from a file on disk."""
  with open(filename) as f:
    return ParsePkgInfo(f.read(), filename, valid_keys, required_keys)
