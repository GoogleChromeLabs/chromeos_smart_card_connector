# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import mock
from mock import patch, Mock
import StringIO

import common
import naclports.__main__
from naclports import error
from naclports.configuration import Configuration


# pylint: disable=no-self-use
class TestMain(common.NaclportsTest):

  def setUp(self):
    super(TestMain, self).setUp()
    self.AddPatch(patch('naclports.util.CheckSDKRoot'))

  @patch('naclports.util.Log', Mock())
  @patch('naclports.util.RemoveTree')
  def testCleanAll(self, mock_rmtree):
    config = Configuration()
    naclports.__main__.CleanAll(config)
    mock_rmtree.assert_any_call('/package/install/path')

  @patch('naclports.__main__.RunMain', Mock(side_effect=error.Error('oops')))
  def testErrorReport(self):
    # Verify that exceptions of the type error.Error are printed
    # to stderr and result in a return code of 1
    with patch('sys.stderr', new_callable=StringIO.StringIO) as stderr:
      self.assertEqual(naclports.__main__.main(None), 1)
    self.assertRegexpMatches(stderr.getvalue(), '^naclports: oops')

  @patch('naclports.__main__.CmdPkgClean')
  def testMainCommandDispatch(self, cmd_pkg_clean):
    mock_pkg = Mock()
    with patch('naclports.source_package.CreatePackage',
               Mock(return_value=mock_pkg)):
      naclports.__main__.RunMain(['clean', 'foo'])
    cmd_pkg_clean.assert_called_once_with(mock_pkg, mock.ANY)

  @patch('naclports.__main__.CmdPkgClean',
         Mock(side_effect=error.DisabledError()))
  def testMainHandlePackageDisabled(self):
    mock_pkg = Mock()
    with patch('naclports.source_package.CreatePackage',
               Mock(return_value=mock_pkg)):
      with self.assertRaises(error.DisabledError):
        naclports.__main__.RunMain(['clean', 'foo'])

  @patch('naclports.__main__.CleanAll')
  def testMainCleanAll(self, clean_all_mock):
    naclports.__main__.RunMain(['clean', '--all'])
    clean_all_mock.assert_called_once_with(Configuration())


class TestCommands(common.NaclportsTest):

  def testListCommand(self):
    config = Configuration()
    pkg = Mock(NAME='foo', VERSION='0.1')
    with patch('naclports.package.InstalledPackageIterator',
               Mock(return_value=[pkg])):
      with patch('sys.stdout', new_callable=StringIO.StringIO) as stdout:
        options = Mock(all=False)
        naclports.__main__.CmdList(config, options, [])
        lines = stdout.getvalue().splitlines()
        self.assertRegexpMatches(lines[0], '^foo\\s+0.1$')
        self.assertEqual(len(lines), 1)

  def testListCommandVerbose(self):
    config = Configuration()
    pkg = Mock(NAME='foo', VERSION='0.1')
    with patch('naclports.package.InstalledPackageIterator',
               Mock(return_value=[pkg])):
      with patch('sys.stdout', new_callable=StringIO.StringIO) as stdout:
        options = Mock(verbose=False, all=False)
        naclports.__main__.CmdList(config, options, [])
        lines = stdout.getvalue().splitlines()
        self.assertRegexpMatches(lines[0], "^foo$")
        self.assertEqual(len(lines), 1)

  @patch('naclports.package.CreateInstalledPackage', Mock())
  def testInfoCommand(self):
    config = Configuration()
    options = Mock()
    file_mock = common.MockFileObject('FOO=bar\n')

    with patch('sys.stdout', new_callable=StringIO.StringIO) as stdout:
      with patch('__builtin__.open', Mock(return_value=file_mock), create=True):
        naclports.__main__.CmdInfo(config, options, ['foo'])
        self.assertRegexpMatches(stdout.getvalue(), "FOO=bar")

  def testContentsCommand(self):
    file_list = ['foo', 'bar']

    options = Mock(verbose=False, all=False)
    package = Mock(NAME='test', Files=Mock(return_value=file_list))

    expected_output = '\n'.join(file_list) + '\n'
    with patch('sys.stdout', new_callable=StringIO.StringIO) as stdout:
      naclports.__main__.CmdPkgContents(package, options)
      self.assertEqual(stdout.getvalue(), expected_output)

    # when the verbose option is set expect CmdContents to output full paths.
    naclports.util.log_level = naclports.util.LOG_VERBOSE
    expected_output = [os.path.join('/package/install/path', f)
                       for f in file_list]
    expected_output = '\n'.join(expected_output) + '\n'
    with patch('sys.stdout', new_callable=StringIO.StringIO) as stdout:
      naclports.__main__.CmdPkgContents(package, options)
      self.assertEqual(stdout.getvalue(), expected_output)
