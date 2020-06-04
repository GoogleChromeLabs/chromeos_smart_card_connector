# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from mock import patch, Mock
import os
import tempfile
import unittest

import common
from naclports.configuration import Configuration
from naclports import util
from naclports import error


class TestUtil(unittest.TestCase):

  def setUp(self):
    common.AddPatch(self, patch('naclports.util.GetSDKRoot',
                                Mock(return_value='/sdk/root')))
    common.AddPatch(self, patch('naclports.util.GetEmscriptenRoot',
                                Mock(return_value='/emscripten/root')))
    common.AddPatch(self, patch('naclports.util.GetPlatform',
                                Mock(return_value='linux')))

  @patch('naclports.util.HashFile', Mock(return_value='sha1'))
  def testVerifyHash(self):
    util.VerifyHash('foo', 'sha1')
    with self.assertRaises(util.HashVerificationError):
      util.VerifyHash('foo', 'sha1x')

  @patch.dict('os.environ', {'PATH': '/x/y/z'})
  def testFindInPath(self):
    with self.assertRaisesRegexp(error.Error, "command not found: foo"):
      util.FindInPath('foo')

    with patch('os.path.exists') as mock_exists:
      with patch('os.path.isfile') as mock_isfile:
        executable = os.path.join('/x/y/z', 'somefile')
        self.assertEqual(util.FindInPath('somefile'), executable)
        mock_exists.assert_called_once_with(executable)
        mock_isfile.assert_called_once_with(executable)

  def testCheckStamp_Missing(self):
    with patch('os.path.exists', Mock(return_value=False)):
      self.assertFalse(util.CheckStamp('foo.stamp', ''))

  def testCheckStamp_Contents(self):
    temp_fd, temp_name = tempfile.mkstemp('naclports_test')
    self.addCleanup(os.remove, temp_name)

    stamp_contents = 'stamp file contents'
    with os.fdopen(temp_fd, 'w') as f:
      f.write(stamp_contents)

    self.assertTrue(util.CheckStamp(temp_name, stamp_contents))
    self.assertFalse(util.CheckStamp(temp_name, stamp_contents + 'x'))

  def testGetInstallRoot(self):
    expected = '/sdk/root/toolchain/linux_pnacl/le32-nacl/usr'
    self.assertEqual(util.GetInstallRoot(Configuration()), expected)

    expected = '/sdk/root/toolchain/linux_x86_glibc/x86_64-nacl/usr'
    self.assertEqual(
        util.GetInstallRoot(Configuration(toolchain='glibc')), expected)

    expected = '/sdk/root/toolchain/linux_pnacl/le32-nacl/usr'
    self.assertEqual(util.GetInstallRoot(Configuration('pnacl')), expected)

    expected = '/emscripten/root/system/local'
    self.assertEqual(util.GetInstallRoot(Configuration('emscripten')), expected)

    expected = '/sdk/root/toolchain/linux_pnacl/x86_64-nacl/usr'
    self.assertEqual(
        util.GetInstallRoot(Configuration(toolchain='clang-newlib')), expected)

  def testHashFile(self):
    temp_name = tempfile.mkstemp('naclports_test')[1]
    self.addCleanup(os.remove, temp_name)

    with self.assertRaises(IOError):
      util.HashFile('/does/not/exist')

    sha1_empty_string = 'da39a3ee5e6b4b0d3255bfef95601890afd80709'
    self.assertEqual(util.HashFile(temp_name), sha1_empty_string)

  @patch('naclports.paths.NACLPORTS_ROOT', '/foo')
  def testRelPath(self):
    self.assertEqual('bar', util.RelPath('/foo/bar'))
    self.assertEqual('../baz/bar', util.RelPath('/baz/bar'))


class TestCheckSDKRoot(TestUtil):

  def testMissingSDKROOT(self):
    with self.assertRaisesRegexp(error.Error, 'NACL_SDK_ROOT does not exist'):
      util.CheckSDKRoot()

  @patch('os.path.exists', Mock())
  @patch('os.path.isdir', Mock())
  @patch('naclports.util.GetSDKVersion', Mock(return_value=10))
  def testSDKVersionCheck(self):
    with patch('naclports.util.MIN_SDK_VERSION', 9):
      util.CheckSDKRoot()

    with patch('naclports.util.MIN_SDK_VERSION', 10):
      util.CheckSDKRoot()

    with patch('naclports.util.MIN_SDK_VERSION', 11):
      with self.assertRaisesRegexp(error.Error, 'requires at least version 11'):
        util.CheckSDKRoot()
