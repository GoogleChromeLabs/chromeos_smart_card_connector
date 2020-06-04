# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import fcntl
import hashlib
import os
import shutil
import subprocess
import sys

# Allow use of this module even if termcolor is missing.  There are many
# standalone python scripts in build_tools that can be run directly without
# PYTHONPATH set (i.e. not via build/python_wrapper that adds this path.
# TODO(sbc): we should probably just assume that all the module dependencies
# are present.
try:
  import termcolor
except ImportError:
  termcolor = None

from naclports import error, paths

GS_URL = 'http://storage.googleapis.com/'
GS_BUCKET = 'naclports'
GS_MIRROR_URL = '%s%s/mirror' % (GS_URL, GS_BUCKET)

# Require the latest version of the NaCl SDK. naclports is built
# and tested against the pepper_canary release. To build aginst older
# versions of the SDK use the one of the pepper_XX branches (or use
# --skip-sdk-version-check).
MIN_SDK_VERSION = 46

arch_to_pkgarch = {
    'x86_64': 'x86-64',
    'i686': 'i686',
    'arm': 'arm',
    'pnacl': 'pnacl',
    'emscripten': 'emscripten',
}

# Inverse of arch_to_pkgarch
pkgarch_to_arch = {v: k for k, v in arch_to_pkgarch.items()}

LOG_ERROR = 0
LOG_WARN = 1
LOG_INFO = 2
LOG_VERBOSE = 3
LOG_TRACE = 4

ELF_MAGIC = '\x7fELF'
PEXE_MAGIC = 'PEXE'

log_level = LOG_INFO
color_mode = 'auto'


def Color(message, color):
  if termcolor and Color.enabled:
    return termcolor.colored(message, color)
  else:
    return message


def CheckStdoutForColorSupport():
  if color_mode == 'auto':
    Color.enabled = sys.stdout.isatty()


def IsElfFile(filename):
  if os.path.islink(filename):
    return False
  with open(filename) as f:
    header = f.read(4)
  return header == ELF_MAGIC


def IsPexeFile(filename):
  if os.path.islink(filename):
    return False
  with open(filename) as f:
    header = f.read(4)
  return header == PEXE_MAGIC


def Memoize(f):
  """Memoization decorator for functions taking one or more arguments."""

  class Memo(dict):

    def __init__(self, f):
      super(Memo, self).__init__()
      self.f = f

    def __call__(self, *args):
      return self[args]

    def __missing__(self, key):
      ret = self[key] = self.f(*key)
      return ret

  return Memo(f)


def SetVerbose(enabled):
  if enabled:
    SetLogLevel(LOG_VERBOSE)
  else:
    SetLogLevel(LOG_INFO)


def SetLogLevel(verbosity):
  global log_level
  log_level = verbosity


def Log(message, verbosity=LOG_INFO):
  """Log a message to the console (stdout)."""
  if log_level < verbosity:
    return
  sys.stdout.write(str(message) + '\n')
  sys.stdout.flush()


def LogHeading(message, suffix=''):
  """Log a colored/highlighted message with optional suffix."""
  if Color.enabled:
    Log(Color(message, 'green') + suffix)
  else:
    if log_level > LOG_WARN:
      # When running in verbose mode make sure heading standout
      Log('###################################################################')
      Log(message + suffix)
      Log('###################################################################')
    else:
      Log(message + suffix)


def Warn(message):
  Log('warning: ' + message, LOG_WARN)


def Trace(message):
  Log(message, LOG_TRACE)


def LogVerbose(message):
  Log(message, LOG_VERBOSE)


def FindInPath(command_name):
  """Search user's PATH for a given executable.

  Returns:
    Full path to executable.
  """
  extensions = ('',)
  if not os.path.splitext(command_name)[1] and os.name == 'nt':
    extensions = ('.bat', '.com', '.exe')

  for path in os.environ.get('PATH', '').split(os.pathsep):
    for ext in extensions:
      full_name = os.path.join(path, command_name + ext)
      if os.path.exists(full_name) and os.path.isfile(full_name):
        return full_name

  raise error.Error('command not found: %s' % command_name)


def DownloadFile(filename, url):
  """Download a file from a given URL.

  Args:
    filename: the name of the file to download the URL to.
    url: then URL to fetch.
  """
  temp_filename = filename + '.partial'
  # Ensure curl is in user's PATH
  FindInPath('curl')
  curl_cmd = ['curl', '--fail', '--location', '--stderr', '-', '-o',
              temp_filename]
  if hasattr(sys.stdout, 'fileno') and os.isatty(sys.stdout.fileno()):
    # Add --progress-bar but only if stdout is a TTY device.
    curl_cmd.append('--progress-bar')
  else:
    # otherwise suppress status output, since curl always assumes its
    # talking to a TTY and writes \r and \b characters.  But add
    # --show-error so that when curl fails it at least prints something.
    curl_cmd += ['--silent', '--show-error']
  curl_cmd.append(url)

  if log_level > LOG_WARN:
    Log('Downloading: %s [%s]' % (url, filename))
  else:
    Log('Downloading: %s' % url.replace(GS_URL, ''))
  try:
    subprocess.check_call(curl_cmd)
  except subprocess.CalledProcessError as e:
    raise error.Error('Error downloading file: %s' % str(e))

  os.rename(temp_filename, filename)


def CheckStamp(filename, contents=None):
  """Check that a given stamp file is up-to-date.

  Returns: False is the file does not exists or is older that that given
    comparison file, or does not contain the given contents. True otherwise.
  """
  if not os.path.exists(filename):
    return False

  if contents is not None:
    with open(filename) as f:
      if not f.read().startswith(contents):
        return False

  return True


@Memoize
def GetSDKRoot():
  """Returns the root of the currently configured Native Client SDK."""
  root = os.environ.get('NACL_SDK_ROOT')
  if root is None:
    local_sdk_root = os.path.join(paths.OUT_DIR, 'nacl_sdk')
    if os.path.exists(local_sdk_root):
      root = local_sdk_root
    else:
      raise error.Error('$NACL_SDK_ROOT not set')
  if sys.platform == "cygwin":
    root = root.replace('\\', '/')
  return root


@Memoize
def GetEmscriptenRoot():
  emscripten = os.environ.get('EMSCRIPTEN')
  if emscripten is None:
    local_root = os.path.join(paths.OUT_DIR, 'emsdk', 'emscripten')
    if os.path.exists(local_root):
      emscripten = local_root
    else:
      raise error.Error('$EMSCRIPTEN not set and %s does not exist.' %
                        local_root)

  if not os.path.isdir(emscripten):
    raise error.Error('$EMSCRIPTEN environment variable does not point'
                      ' to a directory: %s' % emscripten)
  return emscripten


def SetupEmscripten():
  if 'EMSCRIPTEN' in os.environ:
    return

  local_root = GetEmscriptenRoot()
  os.environ['EMSCRIPTEN'] = local_root
  os.environ['EM_CONFIG'] = os.path.join(os.path.dirname(local_root),
                                         '.emscripten')
  try:
    FindInPath('node')
  except error.Error:
    node_bin = os.path.join(paths.OUT_DIR, 'node', 'bin')
    if not os.path.isdir(node_bin):
      raise error.Error('node not found in path and default path not found: %s'
                        % node_bin)

    os.environ['PATH'] += ':' + node_bin
    FindInPath('node')


@Memoize
def GetSDKVersion():
  """Returns the version (as a string) of the current SDK."""
  getos = os.path.join(GetSDKRoot(), 'tools', 'getos.py')
  version = subprocess.check_output([getos, '--sdk-version']).strip()
  return version


def CheckSDKVersion(version):
  """Returns True if the currently configured SDK is 'version' or above."""
  return int(GetSDKVersion()) >= int(version)


@Memoize
def GetSDKRevision():
  """Returns the revision of the currently configured Native Client SDK."""
  getos = os.path.join(GetSDKRoot(), 'tools', 'getos.py')
  version = subprocess.check_output([getos, '--sdk-revision']).strip()
  return int(version)


@Memoize
def GetPlatform():
  """Returns the current platform name according getos.py."""
  getos = os.path.join(GetSDKRoot(), 'tools', 'getos.py')
  platform = subprocess.check_output([getos]).strip()
  return platform

@Memoize
def GetToolchainRoot(config):
  """Returns the toolchain folder for a given NaCl toolchain."""
  if config.toolchain == 'emscripten':
    return GetEmscriptenRoot()

  platform = GetPlatform()
  if config.toolchain in ('pnacl', 'clang-newlib'):
    tc_dir = os.path.join('%s_pnacl' % platform)
  else:
    tc_arch = {'arm': 'arm', 'i686': 'x86', 'x86_64': 'x86'}[config.arch]
    tc_dir = '%s_%s_%s' % (platform, tc_arch, config.libc)

  return os.path.join(GetSDKRoot(), 'toolchain', tc_dir)


@Memoize
def GetInstallRoot(config):
  """Returns the naclports install location given NaCl configuration."""
  tc_dir = GetToolchainRoot(config)

  if config.toolchain == 'emscripten':
    return os.path.join(tc_dir, 'system', 'local')

  if config.toolchain == 'pnacl':
    tc_dir = os.path.join(tc_dir, 'le32-nacl')
  else:
    tc_dir = os.path.join(tc_dir, '%s-nacl' % config.arch)
  return os.path.join(tc_dir, 'usr')


@Memoize
def GetInstallStampRoot(config):
  """Returns the installation metadata folder for the give configuration."""
  tc_root = GetInstallRoot(config)
  return os.path.join(tc_root, 'var', 'lib', 'npkg')


@Memoize
def GetStrip(config):
  tc_dir = GetToolchainRoot(config)
  if config.toolchain == 'pnacl':
    strip = os.path.join(tc_dir, 'bin', 'pnacl-strip')
  else:
    strip = os.path.join(tc_dir, 'bin', '%s-nacl-strip' % config.arch)
  assert os.path.exists(strip), 'strip executable not found: %s' % strip
  return strip


def GetInstallStamp(package_name, config):
  """Returns the filename of the install stamp for for a given package.

  This file is written at install time and contains metadata
  about the installed package.
  """
  root = GetInstallStampRoot(config)
  return os.path.join(root, package_name + '.info')


def GetListFile(package_name, config):
  """Returns the filename of the list of installed files for a given package.

  This file is written at install time.
  """
  root = GetInstallStampRoot(config)
  return os.path.join(root, package_name + '.list')


def IsInstalled(package_name, config, stamp_content=None):
  """Returns True if the given package is installed."""
  stamp = GetInstallStamp(package_name, config)
  result = CheckStamp(stamp, stamp_content)
  return result


def CheckSDKRoot():
  """Check validity of NACL_SDK_ROOT."""
  root = GetSDKRoot()

  if not os.path.isdir(root):
    raise error.Error('$NACL_SDK_ROOT does not exist: %s' % root)

  landmark = os.path.join(root, 'tools', 'getos.py')
  if not os.path.exists(landmark):
    raise error.Error("$NACL_SDK_ROOT (%s) doesn't look right. "
                      "Couldn't find landmark file (%s)" % (root, landmark))

  if not CheckSDKVersion(MIN_SDK_VERSION):
    raise error.Error(
        'This version of naclports requires at least version %s of\n'
        'the NaCl SDK. The version in $NACL_SDK_ROOT is %s. If you want\n'
        'to use naclports with an older version of the SDK please checkout\n'
        'one of the pepper_XX branches (or run with\n'
        '--skip-sdk-version-check).' % (MIN_SDK_VERSION, GetSDKVersion()))


def HashFile(filename):
  """Return the SHA1 (in hex format) of the contents of the given file."""
  block_size = 100 * 1024
  sha1 = hashlib.sha1()
  with open(filename) as f:
    while True:
      data = f.read(block_size)
      if not data:
        break
      sha1.update(data)
  return sha1.hexdigest()


class HashVerificationError(error.Error):
  pass


def VerifyHash(filename, sha1):
  """Return True if the sha1 of the given file match the sha1 passed in."""
  file_sha1 = HashFile(filename)
  if sha1 != file_sha1:
    raise HashVerificationError(
        'verification failed: %s\nExpected: %s\nActual: %s' %
        (filename, sha1, file_sha1))


def RemoveTree(directory):
  """Recursively remove a directory and its contents."""
  if not os.path.exists(directory):
    return
  if not os.path.isdir(directory):
    raise error.Error('RemoveTree: not a directory: %s', directory)
  shutil.rmtree(directory)


def RelPath(filename):
  """Return a pathname relative to the root the naclports src tree.

  This is used mostly to make output more readable when printing filenames."""
  return os.path.relpath(filename, paths.NACLPORTS_ROOT)


def Makedirs(directory):
  if os.path.isdir(directory):
    return
  if os.path.exists(directory):
    raise error.Error('mkdir: File exists and is not a directory: %s' %
                      directory)
  Trace("mkdir: %s" % directory)
  os.makedirs(directory)


class Lock(object):
  """Per-directory flock()-based context manager

  This class will raise an exception if another process already holds the
  lock for the given directory.
  """

  def __init__(self, lock_dir):
    if not os.path.exists(lock_dir):
      Makedirs(lock_dir)
    self.file_name = os.path.join(lock_dir, 'naclports.lock')
    self.fd = open(self.file_name, 'w')

  def __enter__(self):
    try:
      fcntl.flock(self.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except Exception:
      raise error.Error("Unable to acquire lock (%s): Is naclports already "
                        "running?" % self.file_name)

  def __exit__(self, exc_type, exc_val, exc_tb):
    os.remove(self.file_name)
    self.fd.close()


class BuildLock(Lock):
  """Lock used when building a package (essentially a lock on OUT_DIR)"""

  def __init__(self):
    super(BuildLock, self).__init__(paths.OUT_DIR)


class InstallLock(Lock):
  """Lock used when installing/uninstalling package"""

  def __init__(self, config):
    root = GetInstallRoot(config)
    super(InstallLock, self).__init__(root)


CheckStdoutForColorSupport()
