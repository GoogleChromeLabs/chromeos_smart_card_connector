# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from mock import patch, Mock
import os
import tempfile

import common
from naclports import error
from naclports import package_index
from naclports.configuration import Configuration

test_index = '''\
NAME=agg-demo
VERSION=0.1
LICENSE=BSD
DEPENDS=(agg)
BUILD_CONFIG=release
BUILD_ARCH=arm
BUILD_TOOLCHAIN=glibc
BUILD_SDK_VERSION=38
BUILD_NACLPORTS_REVISION=98765
BIN_URL=http://storage.googleapis.com/naclports/builds/pepper_38/1414/packages/agg-demo_0.1_arm_glibc.tar.bz2
BIN_SIZE=10240
BIN_SHA1=f300618f52188a291804dd60d6a5e04361c0ffe6

NAME=agg-demo
VERSION=0.1
LICENSE=BSD
DEPENDS=(agg)
BUILD_CONFIG=release
BUILD_ARCH=i686
BUILD_TOOLCHAIN=glibc
BUILD_SDK_VERSION=38
BUILD_NACLPORTS_REVISION=98765
BIN_URL=http://storage.googleapis.com/naclports/builds/pepper_38/1414/packages/agg-demo_0.1_i686_glibc.tar.bz2
BIN_SIZE=10240
BIN_SHA1=0cb0d2d1380831b38c2b8461528836aa7992435f
'''

test_info = '''\
NAME=foo
VERSION=bar
BUILD_ARCH=arm
BUILD_CONFIG=debug
BUILD_TOOLCHAIN=glibc
BUILD_SDK_VERSION=123
BUILD_NACLPORTS_REVISION=98765
'''


class TestPackageIndex(common.NaclportsTest):

  def testParsingInvalid(self):
    contents = 'FOO=bar\nBAR=baz\n'
    expected_error = "Invalid key 'FOO' in info file dummy_file:1"
    with self.assertRaisesRegexp(error.Error, expected_error):
      package_index.PackageIndex('dummy_file', contents)

  def testParsingValid(self):
    index = package_index.PackageIndex('dummy_file', test_index)
    arm_config = Configuration('arm', 'glibc', False)
    i686_config = Configuration('i686', 'glibc', False)
    self.assertEqual(len(index.packages), 2)
    self.assertTrue(index.Contains('agg-demo', arm_config))
    self.assertTrue(index.Contains('agg-demo', i686_config))

  def testContains(self):
    # Create an empty package index and add a single entry to it
    index = package_index.PackageIndex('dummy_file', '')
    config_debug = Configuration('arm', 'glibc', True)
    config_release = Configuration('arm', 'glibc', False)
    self.assertFalse(index.Contains('foo', config_release))
    index.packages[('foo', config_release)] = {
        'NAME': 'dummy',
        'BUILD_SDK_VERSION': 123
    }
    with patch('naclports.util.GetSDKVersion') as mock_version:
      # Setting the mock SDK version to 123 should mean that the
      # index contains the 'foo' package and it is installable'
      mock_version.return_value = 123
      self.assertTrue(index.Contains('foo', config_release))
      self.assertTrue(index.Installable('foo', config_release))

      # Setting the mock SDK version to some other version should
      # mean the index contains that package but it is not installable.
      mock_version.return_value = 124
      self.assertTrue(index.Contains('foo', config_release))
      self.assertFalse(index.Installable('foo', config_release))

      self.assertFalse(index.Contains('foo', config_debug))
      self.assertFalse(index.Contains('bar', config_release))

  @patch('naclports.util.Log', Mock())
  @patch('naclports.package_index.PREBUILT_ROOT', os.getcwd())
  @patch('naclports.util.VerifyHash', Mock())
  @patch('naclports.util.DownloadFile')
  def testDownload(self, download_file_mock):
    index = package_index.PackageIndex('dummy_file', test_index)
    arm_config = Configuration('arm', 'glibc', False)
    index.Download('agg-demo', arm_config)
    self.assertEqual(download_file_mock.call_count, 1)

  @patch('naclports.util.HashFile', Mock(return_value='sha1'))
  @patch('os.path.getsize', Mock(return_value=100))
  def testWriteIndex(self):
    temp_file = tempfile.mkstemp('naclports_test')[1]
    self.addCleanup(os.remove, temp_file)

    with patch('naclports.package_index.ExtractPkgInfo',
               Mock(return_value=test_info)):
      package_index.WriteIndex(temp_file, (('pkg1', 'url1'), ('pkg2', 'url2')))
