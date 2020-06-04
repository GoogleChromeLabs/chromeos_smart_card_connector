#!/usr/bin/env python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Download the very lastet version Native Client SDK.

This script downloads toolchain bz2's and expands them.

On Windows this script also required access to the cygtar python
module which gets included by the gclient DEPS, and assumes
that cygwin is installed in /cygwin.
"""

import argparse
import os
import subprocess
import sys
import tarfile
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACLPORTS_ROOT = os.path.dirname(SCRIPT_DIR)
sys.path.append(os.path.join(NACLPORTS_ROOT, 'lib'))

import naclports
import naclports.source_package

HISTORY_SIZE = 500

if sys.version_info < (2, 6, 0):
  sys.stderr.write('python 2.6 or later is required run this script\n')
  sys.exit(1)

if sys.version_info >= (3, 0, 0):
  sys.stderr.write('This script does not support python 3.\n')
  sys.exit(1)

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.dirname(SCRIPT_DIR)
OUT_DIR = os.path.join(SRC_DIR, 'out')
TARGET_DIR = os.path.join(OUT_DIR, 'nacl_sdk')

if sys.platform == 'win32':
  NACL_BUILD_DIR = os.path.join(SRC_DIR, 'native_client', 'build')
  sys.path.append(NACL_BUILD_DIR)
  import cygtar  # pylint: disable=import-error

BOT_GSUTIL = '/b/build/scripts/slave/gsutil'
GS_URL_BASE = 'gs://nativeclient-mirror/nacl/nacl_sdk/'
GSTORE = 'http://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/'


def DetermineSDKURL(flavor, base_url, version):
  """Download one Native Client toolchain and extract it.

  Arguments:
    flavor: flavor of the sdk to download
    base_url: base url to download toolchain tarballs from
    version: version directory to select tarballs from

  Returns:
    The URL of the SDK archive
  """
  # gsutil.py ships with depot_tools, which should be in PATH
  gsutil = [sys.executable, naclports.util.FindInPath('gsutil.py')]
  path = flavor + '.tar.bz2'

  def GSList(path):
    """Run gsutil 'ls' on a path and return just the basenames of the
    elements within.
    """
    cmd = gsutil + ['ls', base_url + path]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    p_stdout = p.communicate()[0]
    if p.returncode:
      raise naclports.Error('gsutil command failed: %s' % str(cmd))

    elements = p_stdout.splitlines()
    return [os.path.basename(os.path.normpath(elem)) for elem in elements]

  if version == 'latest':
    naclports.Log('Looking for latest SDK build...')
    # List the top level of the nacl_sdk folder
    versions = GSList('')
    # Find all trunk revision
    versions = [v for v in versions if v.startswith('trunk')]

    # Look backwards through all trunk revisions
    # Only look back HISTORY_SIZE revisions so this script doesn't take
    # forever.
    versions = list(reversed(sorted(versions)))
    for version_dir in versions[:HISTORY_SIZE]:
      contents = GSList(version_dir)
      if path in contents:
        version = version_dir.rsplit('.', 1)[1]
        break
    else:
      raise naclports.Error('No SDK build (%s) found in last %d trunk builds' %
                            (path, HISTORY_SIZE))

  return '%strunk.%s/%s' % (GSTORE, version, path)


def Untar(bz2_filename):
  if sys.platform == 'win32':
    tar_file = None
    try:
      naclports.Log('Unpacking tarball...')
      tar_file = cygtar.CygTar(bz2_filename, 'r:bz2')
      tar_file.Extract()
    except Exception, err:
      raise naclports.Error('Error unpacking %s' % str(err))
    finally:
      if tar_file:
        tar_file.Close()
  else:
    if subprocess.call(['tar', 'jxf', bz2_filename]):
      raise naclports.Error('Error unpacking')


def FindCygwin():
  if os.path.exists(r'\cygwin'):
    return r'\cygwin'
  elif os.path.exists(r'C:\cygwin'):
    return r'C:\cygwin'
  else:
    raise naclports.Error(r'failed to find cygwin in \cygwin or c:\cygwin')


def DownloadAndInstallSDK(url, target_dir):
  bz2_dir = OUT_DIR
  if not os.path.exists(bz2_dir):
    os.makedirs(bz2_dir)
  bz2_filename = os.path.join(bz2_dir, url.split('/')[-1])

  if sys.platform in ['win32', 'cygwin']:
    cygbin = os.path.join(FindCygwin(), 'bin')

  # Download it.
  naclports.DownloadFile(bz2_filename, url)

  # Extract toolchain.
  old_cwd = os.getcwd()
  os.chdir(bz2_dir)
  Untar(bz2_filename)
  os.chdir(old_cwd)

  # Calculate pepper_dir by taking common prefix of tar
  # file contents
  tar = tarfile.open(bz2_filename)
  names = tar.getnames()
  tar.close()
  pepper_dir = os.path.commonprefix(names)

  actual_dir = os.path.join(bz2_dir, pepper_dir)

  # Drop old versions.
  if os.path.exists(target_dir):
    naclports.Log('Cleaning up old SDK...')
    if sys.platform in ['win32', 'cygwin']:
      cmd = [os.path.join(cygbin, 'bin', 'rm.exe'), '-rf']
    else:
      cmd = ['rm', '-rf']
    cmd.append(target_dir)
    returncode = subprocess.call(cmd)
    assert returncode == 0

  naclports.Log('Renaming toolchain "%s" -> "%s"' % (actual_dir, target_dir))
  os.rename(actual_dir, target_dir)

  if sys.platform in ['win32', 'cygwin']:
    time.sleep(2)  # Wait for windows.

  # Clean up: remove the sdk bz2.
  os.remove(bz2_filename)

  naclports.Log('Install complete.')


PLATFORM_COLLAPSE = {
    'win32': 'win',
    'cygwin': 'win',
    'linux': 'linux',
    'linux2': 'linux',
    'darwin': 'mac',
}


def main(argv):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('-v', '--version', default='latest',
      help='which version of the SDK to download')
  parser.add_argument('--bionic', action='store_true',
      help='download bionic version of the SDK (linux only).')
  options = parser.parse_args(argv)

  if options.bionic:
    flavor = 'naclsdk_bionic'
  else:
    flavor = 'naclsdk_' + PLATFORM_COLLAPSE[sys.platform]

  url = DetermineSDKURL(flavor, base_url=GS_URL_BASE, version=options.version)

  stamp_file = os.path.join(TARGET_DIR, 'stamp')
  if os.path.exists(stamp_file):
    with open(stamp_file) as f:
      installed_url = f.read().strip()
      if installed_url == url:
        naclports.Log('SDK already installed: %s' % url)
        return 0
      else:
        naclports.Log('Ignoring currently installed SDK: %s' % installed_url)

  DownloadAndInstallSDK(url, TARGET_DIR)
  with open(stamp_file, 'w') as f:
    f.write(url + '\n')
  return 0


if __name__ == '__main__':
  try:
    rtn = main(sys.argv[1:])
  except naclports.Error as e:
    sys.stderr.write("error: %s\n" % str(e))
    rtn = 1
  sys.exit(rtn)
