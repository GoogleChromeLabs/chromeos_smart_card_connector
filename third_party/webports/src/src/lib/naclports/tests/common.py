# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from mock import Mock, patch
import unittest


def MockFileObject(contents=''):
  file_mock = Mock(name="file_mock", spec=file)
  file_mock.read.return_value = contents
  file_mock.__enter__ = lambda s: s
  file_mock.__exit__ = Mock(return_value=False)
  return file_mock


def AddPatch(testcase, patcher):
  patcher.start()
  testcase.addCleanup(patcher.stop)


class NaclportsTest(unittest.TestCase):
  """Class that sets up core mocks common to all test cases."""

  AddPatch = AddPatch

  def setUp(self):
    AddPatch(self, patch.dict('os.environ', {'NACL_SDK_ROOT': '/sdk/root'}))
    AddPatch(self, patch('naclports.util.GetPlatform',
                         Mock(return_value='linux')))
    AddPatch(self, patch('naclports.util.GetInstallRoot',
                         Mock(return_value='/package/install/path')))
    AddPatch(self, patch('naclports.util.GetSDKRoot',
                         Mock(return_value='/sdk/root')))

    mock_lock = Mock()
    mock_lock.__enter__ = lambda s: s
    mock_lock.__exit__ = Mock(return_value=False)
    AddPatch(self, patch('naclports.util.InstallLock',
                         Mock(return_value=mock_lock)))

    mock_lock = Mock()
    mock_lock.__enter__ = lambda s: s
    mock_lock.__exit__ = Mock(return_value=False)
    AddPatch(self, patch('naclports.util.BuildLock',
                         Mock(return_value=mock_lock)))
