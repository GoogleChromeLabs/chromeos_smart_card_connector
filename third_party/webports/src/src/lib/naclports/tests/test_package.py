# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from naclports import error
from naclports import package

test_info = '''\
NAME=foo
VERSION=0.1
'''


class TestPackage(unittest.TestCase):

  def testValidLibcDisabled(self):  # pylint: disable=no-self-use
    pkg = package.Package()
    pkg.ParseInfo(test_info + 'DISABLED_LIBC=(newlib glibc)')

  def testInvalidLibcDisabled(self):
    pkg = package.Package()
    # clang-newlib is TOOLCHAIN, not a LIBC
    with self.assertRaisesRegexp(error.Error, 'invalid libc: clang-newlib'):
      pkg.ParseInfo(test_info + 'DISABLED_LIBC=(clang-newlib)')

  def testValidToolchainDisabled(self):  # pylint: disable=no-self-use
    pkg = package.Package()
    pkg.ParseInfo(test_info + 'DISABLED_TOOLCHAIN=(pnacl glibc clang-newlib)')

  def testInvalidToolchainDisabled(self):
    pkg = package.Package()
    with self.assertRaisesRegexp(error.Error, 'invalid toolchain: foo'):
      pkg.ParseInfo(test_info + 'DISABLED_TOOLCHAIN=(foo)')

  def testValidArchDisabled(self):  # pylint: disable=no-self-use
    pkg = package.Package()
    pkg.ParseInfo(test_info + 'DISABLED_ARCH=(arm i686 x86_64)')

  def testInvalidArchDisabled(self):
    pkg = package.Package()
    with self.assertRaisesRegexp(error.Error, 'invalid architecture: foo'):
      pkg.ParseInfo(test_info + 'DISABLED_ARCH=(foo)')
