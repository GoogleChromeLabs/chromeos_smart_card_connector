# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from mock import patch, Mock
import mock
import tempfile
import textwrap
import shutil
import os

import common
from naclports import source_package
from naclports import error
from naclports import paths
from naclports.configuration import Configuration


class TestSourcePackage(common.NaclportsTest):

  def setUp(self):
    super(TestSourcePackage, self).setUp()
    self.tempdir = tempfile.mkdtemp(prefix='naclports_test_')
    self.addCleanup(shutil.rmtree, self.tempdir)
    self.temp_ports = os.path.join(self.tempdir, 'ports')

    self.AddPatch(patch('naclports.paths.NACLPORTS_ROOT', self.tempdir))
    self.AddPatch(patch('naclports.paths.BUILD_ROOT',
                        os.path.join(self.tempdir, 'build_root')))
    self.AddPatch(patch('naclports.paths.CACHE_ROOT',
                        os.path.join(self.tempdir, 'cache')))
    self.AddPatch(patch('naclports.paths.OUT_DIR',
                        os.path.join(self.tempdir, 'out_dir')))
    self.AddPatch(patch('naclports.paths.STAMP_DIR',
                        os.path.join(self.tempdir, 'stamp_dir')))

  def CreateTestPackage(self, name, extra_info=''):
    """Creates a source package directory in a temporary directory.

    Args:
      name: The name of the temporary package.
      extra_info: extra information to append to the pkg_info file.

    Returns:
      The new package source directory
    """
    pkg_root = os.path.join(self.temp_ports, name)
    os.makedirs(pkg_root)
    with open(os.path.join(pkg_root, 'pkg_info'), 'w') as info:
      info.write("NAME=%s\nVERSION=1.0\n%s" % (name, extra_info))
    return pkg_root

  def testStampContentsMatch(self):
    stamp_file = os.path.join(self.tempdir, 'test_stamp')
    # stamp does not exist
    self.assertFalse(source_package.StampContentsMatch(stamp_file, ''))

    open(stamp_file, 'w').close()
    self.assertTrue(source_package.StampContentsMatch(stamp_file, ''))
    self.assertFalse(source_package.StampContentsMatch(stamp_file, 'foo'))

  def testInvalidSourceDir(self):
    """test that invalid source directory generates an error."""
    path = '/bad/path'
    expected_error = 'Invalid package folder: ' + path
    with self.assertRaisesRegexp(error.Error, expected_error):
      source_package.SourcePackage(path)

  def testValidSourceDir(self):
    """test that valid source directory is loaded correctly."""
    root = self.CreateTestPackage('foo')
    pkg = source_package.SourcePackage(root)
    self.assertEqual(pkg.NAME, 'foo')
    self.assertEqual(pkg.root, root)

  def testIsBuiltMalformedBinary(self):
    """test that IsBuilt() can handle malformed package files."""
    root = self.CreateTestPackage('foo')
    pkg = source_package.SourcePackage(root)
    invalid_binary = os.path.join(self.tempdir, 'package.tar.bz2')
    with open(invalid_binary, 'w') as f:
      f.write('this is not valid package file\n')
    pkg.PackageFile = Mock(return_value=invalid_binary)
    self.assertFalse(pkg.IsBuilt())

  @patch('naclports.source_package.SourcePackage.RunBuildSh',
         Mock(return_value=True))
  @patch('naclports.source_package.Log', Mock())
  def testBuildPackage(self):
    root = self.CreateTestPackage('foo')
    pkg = source_package.SourcePackage(root)
    pkg.Build(True)

  def testSourcePackageIterator(self):
    self.CreateTestPackage('foo')
    pkgs = [p for p in source_package.SourcePackageIterator()]
    self.assertEqual(len(pkgs), 1)
    self.assertEqual(pkgs[0].NAME, 'foo')

  def testGetBuildLocation(self):
    root = self.CreateTestPackage('foo')
    pkg = source_package.SourcePackage(root)
    location = pkg.GetBuildLocation()
    self.assertTrue(location.startswith(paths.BUILD_ROOT))
    self.assertEqual(os.path.basename(location), '%s-%s' %
                     (pkg.NAME, pkg.VERSION))

  @patch('naclports.util.Log', Mock())
  def testExtract(self):
    root = self.CreateTestPackage('foo', 'URL=someurl.tar.gz\nSHA1=123')
    pkg = source_package.SourcePackage(root)

    def fake_extract(archive, dest):
      os.mkdir(os.path.join(dest, '%s-%s' % (pkg.NAME, pkg.VERSION)))

    with patch('naclports.source_package.ExtractArchive', fake_extract):
      pkg.Extract()

  def testCreatePackageInvalid(self):
    with self.assertRaisesRegexp(error.Error, 'Package not found: foo'):
      source_package.CreatePackage('foo')

  def testFormatTimeDelta(self):
    expectations = (
       (1, '1s'),
       (60, '1m'),
       (70, '1m10s'),
    )

    for secs, expected_result in expectations:
      self.assertEqual(expected_result, source_package.FormatTimeDelta(secs))

  def testConflicts(self):
    root = self.CreateTestPackage('foo', 'CONFLICTS=(bar)')
    pkg = source_package.SourcePackage(root)

    # with no other packages installed
    with patch('naclports.util.IsInstalled', Mock(return_value=False)):
      pkg.CheckInstallable()

    # with all possible packages installed
    with patch('naclports.util.IsInstalled') as is_installed:
      is_installed.return_value = True
      with self.assertRaises(source_package.PkgConflictError):
        pkg.CheckInstallable()
      is_installed.assert_called_once_with('bar', pkg.config)

  def testDisabled(self):
    root = self.CreateTestPackage('foo', 'DISABLED=1')
    pkg = source_package.SourcePackage(root)
    with self.assertRaisesRegexp(error.DisabledError, 'package is disabled'):
      pkg.CheckInstallable()

  def testDisabledArch(self):
    self.CreateTestPackage('bar', 'DISABLED_ARCH=(x86_64)')

    pkg = source_package.CreatePackage(
        'bar', config=Configuration(toolchain='clang-newlib'))
    with self.assertRaisesRegexp(error.DisabledError,
                                 'disabled for architecture: x86_64'):
      pkg.CheckInstallable()

  def testSingleArch(self):
    self.CreateTestPackage('bar', 'ARCH=(arm)')

    pkg = source_package.CreatePackage('bar')
    with self.assertRaisesRegexp(error.DisabledError,
                                 'disabled for architecture: pnacl$'):
      pkg.CheckInstallable()

  def testDisabledLibc(self):
    self.CreateTestPackage('bar', 'DISABLED_LIBC=(newlib)')

    pkg = source_package.CreatePackage('bar')
    with self.assertRaisesRegexp(error.DisabledError,
                                 'cannot be built with newlib$'):
      pkg.CheckInstallable()

  def testDisabledToolchain(self):
    self.CreateTestPackage('bar', 'DISABLED_TOOLCHAIN=(pnacl)')

    pkg = source_package.CreatePackage('bar')
    with self.assertRaisesRegexp(error.DisabledError,
                                 'cannot be built with pnacl$'):
      pkg.CheckInstallable()

  def testDisabledToolchainArch(self):
    self.CreateTestPackage('bar', 'DISABLED_TOOLCHAIN=(glibc/x86_64)')

    pkg = source_package.CreatePackage(
        'bar', config=Configuration(toolchain='glibc'))
    with self.assertRaisesRegexp(
        error.DisabledError, 'cannot be built with glibc for x86_64$'):
      pkg.CheckInstallable()

    self.CreateTestPackage('bar2', 'DISABLED_TOOLCHAIN=(pnacl/arm)')

    pkg = source_package.CreatePackage('bar2')
    pkg.CheckInstallable()

  def testCheckInstallableDepends(self):
    self.CreateTestPackage('foo', 'DEPENDS=(bar)')
    self.CreateTestPackage('bar', 'DISABLED=1')

    pkg = source_package.CreatePackage('foo')
    with self.assertRaisesRegexp(error.DisabledError,
                                 'bar: package is disabled$'):
      pkg.CheckInstallable()

  def testCheckBuildable(self):
    self.CreateTestPackage('foo', 'BUILD_OS=solaris')

    pkg = source_package.CreatePackage('foo')
    with self.assertRaisesRegexp(error.DisabledError,
                                 'can only be built on solaris$'):
      pkg.CheckBuildable()

  @patch('naclports.util.GetSDKVersion', Mock(return_value=123))
  def testMinSDKVersion(self):
    self.CreateTestPackage('foo', 'MIN_SDK_VERSION=123')
    pkg = source_package.CreatePackage('foo')
    pkg.CheckBuildable()

    self.CreateTestPackage('foo2', 'MIN_SDK_VERSION=121')
    pkg = source_package.CreatePackage('foo2')
    pkg.CheckBuildable()

    self.CreateTestPackage('foo3', 'MIN_SDK_VERSION=124')
    pkg = source_package.CreatePackage('foo3')
    with self.assertRaisesRegexp(error.DisabledError,
                                 'requires SDK version 124 or above'):
      pkg.CheckBuildable()

  @patch('naclports.util.GetSDKVersion', Mock(return_value=1234))
  def testInstalledInfoContents(self):
    root = self.CreateTestPackage('foo')
    pkg = source_package.SourcePackage(root)
    expected_contents = textwrap.dedent('''\
      NAME=foo
      VERSION=1.0
      BUILD_CONFIG=release
      BUILD_ARCH=pnacl
      BUILD_TOOLCHAIN=pnacl
      BUILD_SDK_VERSION=1234
      ''')
    self.assertRegexpMatches(pkg.InstalledInfoContents(), expected_contents)

  def testRunGitCmd_BadRepo(self):
    os.mkdir(os.path.join(self.tempdir, '.git'))
    with self.assertRaisesRegexp(error.Error, 'git command failed'):
      source_package.InitGitRepo(self.tempdir)

  def testInitGitRepo(self):
    # init a git repo containing a single dummy file
    with open(os.path.join(self.tempdir, 'file'), 'w') as f:
      f.write('bar')
    source_package.InitGitRepo(self.tempdir)
    self.assertTrue(os.path.isdir(os.path.join(self.tempdir, '.git')))

    # InitGitRepo should work on existing git repositories too.
    source_package.InitGitRepo(self.tempdir)

  @patch('naclports.util.DownloadFile')
  @patch('naclports.util.VerifyHash')
  def testDownload(self, mock_verify, mock_download):
    self.CreateTestPackage('foo', 'URL=foo/bar.tar.gz\nSHA1=X123')
    pkg = source_package.CreatePackage('foo')
    pkg.Download()
    expected_filename = os.path.join(paths.CACHE_ROOT, 'bar.tar.gz')
    mock_download.assert_called_once_with(expected_filename, mock.ANY)
    mock_verify.assert_called_once_with(expected_filename, 'X123')

  @patch('naclports.util.DownloadFile')
  def testDownloadMissingSHA1(self, mock_download):
    self.CreateTestPackage('foo', 'URL=foo/bar')
    pkg = source_package.CreatePackage('foo')
    with self.assertRaisesRegexp(error.Error, 'missing SHA1 attribute'):
      pkg.Download()
