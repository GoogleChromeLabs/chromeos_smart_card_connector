# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from mock import Mock, patch, call
import textwrap

import common
from naclports import binary_package


# pylint: disable=no-self-use
class TestBinaryPackage(common.NaclportsTest):

  @patch('os.rename')
  @patch('os.makedirs')
  @patch('os.path.isdir', Mock(return_value=False))
  def testInstallFile(self, makedirs_mock, rename_mock):
    mock_file = common.MockFileObject()
    with patch('__builtin__.open', Mock(return_value=mock_file)):
      binary_package.InstallFile('fname', 'location1', 'location2')
      makedirs_mock.assert_called_once_with('location2')
      rename_mock.assert_has_calls([call('location1/fname', 'location2/fname')])

  def testRelocateFile(self):
    # Only certain files should be relocated. A file called 'testfile'
    # for example, should not be touched.
    with patch('__builtin__.open', Mock(), create=True) as open_mock:
      binary_package.RelocateFile('testfile', 'newroot')
      open_mock.assert_not_called()

  @patch('naclports.binary_package.BinaryPackage.VerifyArchiveFormat', Mock())
  @patch('naclports.binary_package.BinaryPackage.GetPkgInfo')
  @patch('naclports.util.GetInstallStamp',
         Mock(return_value='stamp_dir/stamp_file'))
  def testWriteStamp(self, mock_get_info):
    fake_binary_pkg_info = textwrap.dedent('''\
      NAME=foo
      VERSION=1.0
      BUILD_CONFIG=release
      BUILD_ARCH=arm
      BUILD_TOOLCHAIN=glibc
      BUILD_SDK_VERSION=38
      BUILD_NACLPORTS_REVISION=1414
      ''')
    mock_get_info.return_value = fake_binary_pkg_info
    pkg = binary_package.BinaryPackage('foo')
    mock_stamp_file = common.MockFileObject()
    with patch('__builtin__.open', Mock(return_value=mock_stamp_file),
               create=True):
      pkg.WriteStamp()
    mock_stamp_file.write.assert_called_once_with(fake_binary_pkg_info)
