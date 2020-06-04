# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import fnmatch
import os
import re
import subprocess
import sys
import tempfile
import time
import urlparse

from naclports import binary_package
from naclports import configuration
from naclports import package
from naclports import package_index
from naclports import util
from naclports import paths
from naclports import bsd_pkg
from naclports.util import Log, Trace, LogVerbose
from naclports.error import Error, DisabledError, PkgFormatError


class PkgConflictError(Error):
  pass


@contextlib.contextmanager
def RedirectStdoutStderr(filename):
  """Context manager that replaces stdout and stderr streams."""
  if filename is None:
    yield
    return

  with open(filename, 'a') as stream:
    old_stdout = sys.stdout
    old_stderr = sys.stderr
    sys.stdout = stream
    sys.stderr = stream
    util.CheckStdoutForColorSupport()
    try:
      yield
    finally:
      sys.stdout = old_stdout
      sys.stderr = old_stderr
      util.CheckStdoutForColorSupport()


def FormatTimeDelta(delta):
  """Converts a duration in seconds to a human readable string.

  Args:
    delta: the amount of time in seconds.

  Returns: A string describing the amount of time passed in.
  """
  rtn = ''
  if delta >= 60:
    mins = delta / 60
    rtn += '%dm' % mins
    delta -= mins * 60

  if delta:
    rtn += '%.0fs' % delta
  return rtn


def ExtractArchive(archive, destination):
  ext = os.path.splitext(archive)[1]
  if ext in ('.gz', '.tgz', '.bz2', '.xz'):
    cmd = ['tar', 'xf', archive, '-C', destination]
  elif ext in ('.zip',):
    cmd = ['unzip', '-q', '-d', destination, archive]
  else:
    raise Error('unhandled extension: %s' % ext)
  LogVerbose(cmd)
  subprocess.check_call(cmd)


def RunGitCmd(directory, cmd, error_ok=False):
  cmd = ['git'] + cmd
  LogVerbose('%s' % ' '.join(cmd))
  p = subprocess.Popen(cmd,
                       cwd=directory,
                       stderr=subprocess.PIPE,
                       stdout=subprocess.PIPE)
  stdout, stderr = p.communicate()
  if not error_ok and p.returncode != 0:
    if stdout:
      Log(stdout)
    if stderr:
      Log(stderr)
    raise Error('git command failed: %s' % cmd)
  Trace('git exited with %d' % p.returncode)
  return p.returncode


def InitGitRepo(directory):
  """Initialize the source git repository for a given package directory.

  This function works for unpacked tar files as well as cloned git
  repositories.  It sets up an 'upstream' branch pointing and the
  pristine upstream sources and a 'master' branch will contain changes
  specific to naclports (normally the result of applying nacl.patch).

  Args:
    directory: Directory containing unpacked package sources.
  """
  git_dir = os.path.join(directory, '.git')

  # If the upstream ref exists then we've already initialized this repo
  if os.path.exists(os.path.join(git_dir, 'refs', 'heads', 'upstream')):
    return

  if os.path.exists(git_dir):
    Log('Init existing git repo: %s' % directory)
    RunGitCmd(directory, ['checkout', '-b', 'placeholder'])
    RunGitCmd(directory, ['branch', '-D', 'upstream'], error_ok=True)
    RunGitCmd(directory, ['branch', '-D', 'master'], error_ok=True)
    RunGitCmd(directory, ['checkout', '-b', 'upstream'])
    RunGitCmd(directory, ['checkout', '-b', 'master'])
    RunGitCmd(directory, ['branch', '-D', 'placeholder'])
  else:
    Log('Init new git repo: %s' % directory)
    RunGitCmd(directory, ['init'])
    try:
      # Setup a bogus identity on the buildbots.
      if os.environ.get('BUILDBOT_BUILDERNAME'):
        RunGitCmd(directory, ['config', 'user.name', 'Naclports'])
        RunGitCmd(directory, ['config', 'user.email', 'nobody@example.com'])
      RunGitCmd(directory, ['add', '-f', '.'])
      RunGitCmd(directory, ['commit', '-m', 'Upstream version'])
      RunGitCmd(directory, ['checkout', '-b', 'upstream'])
      RunGitCmd(directory, ['checkout', 'master'])
    except:  # pylint: disable=bare-except
      # If git setup fails or is interrupted then remove the partially
      # initialized repository.
      util.RemoveTree(os.path.join(git_dir))


def WriteStamp(stamp_file, stamp_contents):
  """Write a stamp file to disk with the given file contents."""
  stamp_dir = os.path.dirname(stamp_file)
  util.Makedirs(stamp_dir)

  with open(stamp_file, 'w') as f:
    f.write(stamp_contents)
  Log('Wrote stamp: %s' % stamp_file)


def StampContentsMatch(stamp_file, stamp_contents):
  """Return True is stamp_file exists and contains the given contents."""
  if not os.path.exists(stamp_file):
    return False
  with open(stamp_file) as f:
    return f.read() == stamp_contents


class SourcePackage(package.Package):
  """Representation of a single naclports source package.

  Package objects correspond to folders on disk which
  contain a 'pkg_info' file.
  """

  def __init__(self, pkg_root, config=None):
    super(SourcePackage, self).__init__()
    if config is None:
      config = configuration.Configuration()
    self.config = config

    self.root = os.path.abspath(pkg_root)
    info_file = os.path.join(self.root, 'pkg_info')
    if not os.path.isdir(self.root) or not os.path.exists(info_file):
      raise Error('Invalid package folder: %s' % pkg_root)

    super(SourcePackage, self).__init__(info_file)
    if self.NAME != os.path.basename(self.root):
      raise Error('%s: package NAME must match directory name' % self.info)

  def GetInstallLocation(self):
    install_dir = 'install_%s' % util.arch_to_pkgarch[self.config.arch]
    if self.config.arch != self.config.toolchain:
       install_dir += '_' + self.config.toolchain
    if self.config.config_name == 'debug':
       install_dir += '_debug'
    return os.path.join(paths.BUILD_ROOT, self.NAME, install_dir, 'payload')

  def GetBuildLocation(self):
    package_dir = self.ARCHIVE_ROOT or '%s-%s' % (self.NAME, self.VERSION)
    return os.path.join(paths.BUILD_ROOT, self.NAME, package_dir)

  def GetPatchFile(self):
    patch_name = self.PATCH_NAME or 'nacl.patch'
    return os.path.join(self.root, patch_name)

  def GetArchiveFilename(self):
    if self.URL_FILENAME:
      return self.URL_FILENAME

    if self.IsGitUpstream() or self.URL is None:
      return None

    return os.path.basename(urlparse.urlparse(self.URL)[2])

  def DownloadLocation(self):
    archive = self.GetArchiveFilename()
    if not archive:
      return
    return os.path.join(paths.CACHE_ROOT, archive)

  def IsInstalled(self):
    return util.IsInstalled(self.NAME, self.config,
                            self.InstalledInfoContents())

  def IsGitUpstream(self):
    return self.URL and self.URL.split('@')[0].endswith('.git')

  def InstallDeps(self, force, from_source=False):
    for dep in self.Dependencies():
      if not dep.IsAnyVersionInstalled() or force == 'all':
        dep.Install(True, force, from_source)

  def PackageFile(self):
    fullname = [os.path.join(paths.PACKAGES_ROOT, self.NAME)]
    fullname.append(self.VERSION)
    fullname.append(util.arch_to_pkgarch[self.config.arch])
    # for pnacl toolchain and arch are the same
    if self.config.toolchain != self.config.arch:
      fullname.append(self.config.toolchain)
    if self.config.debug:
      fullname.append('debug')
    return '_'.join(fullname) + '.tar.bz2'

  def InstalledInfoContents(self):
    """Generate content of the .info file to install based on source
    pkg_info file and current build configuration."""
    with open(self.info) as f:
      info_content = f.read()
    info_content += 'BUILD_CONFIG=%s\n' % self.config.config_name
    info_content += 'BUILD_ARCH=%s\n' % self.config.arch
    info_content += 'BUILD_TOOLCHAIN=%s\n' % self.config.toolchain
    info_content += 'BUILD_SDK_VERSION=%s\n' % util.GetSDKVersion()
    return info_content

  def IsBuilt(self):
    package_file = self.PackageFile()
    if not os.path.exists(package_file):
      return False
    try:
      pkg = binary_package.BinaryPackage(package_file)
    except PkgFormatError:
      # If the package is malformed in some way or in some old format
      # then treat it as not built.
      return False
    return pkg.IsInstallable()

  def Install(self, build_deps, force=None, from_source=False):
    self.CheckInstallable()

    if force is None and self.IsInstalled():
      self.LogStatus('Already installed')
      return

    if build_deps:
      self.InstallDeps(force, from_source)

    if force:
      from_source = True

    package_file = self.PackageFile()
    if not self.IsBuilt() and not from_source:
      index = package_index.GetCurrentIndex()
      if index.Installable(self.NAME, self.config):
        package_file = index.Download(self.NAME, self.config)
      else:
        from_source = True

    if from_source:
      self.Build(build_deps, force)

    if self.IsAnyVersionInstalled():
      installed_pkg = self.GetInstalledPackage()
      installed_pkg.LogStatus('Uninstalling existing')
      installed_pkg.DoUninstall()

    binary_package.BinaryPackage(package_file).Install(force)

  def GetInstalledPackage(self):
    return package.CreateInstalledPackage(self.NAME, self.config)

  def CreatePkgFile(self):
    """Create and pkg file for use with the FreeBSD pkg tool.

    Create a package from the result of the package's InstallStep.
    """
    install_dir = self.GetInstallLocation()
    if not os.path.exists(install_dir):
      Log('Skiping pkg creation. Install dir not found: %s' % install_dir)
      return

    # Strip all elf or pexe files in the install directory (except .o files
    # since we don't want to strip, for example, crt1.o)
    if not self.config.debug and self.config.toolchain != 'emscripten':
      strip = util.GetStrip(self.config)
      for root, _, files in os.walk(install_dir):
        for filename in files:
          fullname = os.path.join(root, filename)
          if (os.path.isfile(fullname) and util.IsElfFile(fullname)
              and os.path.splitext(fullname)[1] != '.o'):
            Log('stripping: %s %s' % (strip, fullname))
            subprocess.check_call([strip, fullname])

    abi = 'pkg_' + self.config.toolchain
    if self.config.arch != self.config.toolchain:
      abi += "_" + util.arch_to_pkgarch[self.config.arch]
    abi_dir = os.path.join(paths.PUBLISH_ROOT, abi)
    pkg_file = os.path.join(abi_dir, '%s-%s.tbz' % (self.NAME,
      self.VERSION))
    util.Makedirs(abi_dir)
    deps = self.DEPENDS
    if self.config.toolchain != 'glibc':
        deps = []
    bsd_pkg.CreatePkgFile(self.NAME, self.VERSION, self.config.arch,
        self.GetInstallLocation(), pkg_file, deps)

  def Build(self, build_deps, force=None):
    self.CheckBuildable()

    if build_deps:
      self.InstallDeps(force)

    if not force and self.IsBuilt():
      self.LogStatus('Already built')
      return

    log_root = os.path.join(paths.OUT_DIR, 'logs')
    util.Makedirs(log_root)

    self.LogStatus('Building')

    if util.log_level > util.LOG_INFO:
      log_filename = None
    else:
      log_filename = os.path.join(log_root, '%s_%s.log' %
                                  (self.NAME,
                                   str(self.config).replace('/', '_')))
      if os.path.exists(log_filename):
        os.remove(log_filename)

    start = time.time()
    with util.BuildLock():
      try:
        with RedirectStdoutStderr(log_filename):
          old_log_level = util.log_level
          util.log_level = util.LOG_VERBOSE
          try:
            self.Download()
            self.Extract()
            self.Patch()
            self.RunBuildSh()
            self.CreatePkgFile()
          finally:
            util.log_level = old_log_level
      except:
        if log_filename:
          with open(log_filename) as log_file:
            sys.stdout.write(log_file.read())
        raise

    duration = FormatTimeDelta(time.time() - start)
    util.LogHeading('Build complete', ' [took %s]' % duration)

  def RunBuildSh(self):
    build_port = os.path.join(paths.TOOLS_DIR, 'build_port.sh')
    cmd = [build_port]

    if self.config.toolchain == 'emscripten':
      util.SetupEmscripten()
    env = os.environ.copy()
    env['TOOLCHAIN'] = self.config.toolchain
    env['NACL_ARCH'] = self.config.arch
    env['NACL_DEBUG'] = self.config.debug and '1' or '0'
    env['NACL_SDK_ROOT'] = util.GetSDKRoot()
    rtn = subprocess.call(cmd,
                          stdout=sys.stdout,
                          stderr=sys.stderr,
                          cwd=self.root,
                          env=env)
    if rtn != 0:
      raise Error("Building %s: failed." % (self.NAME))

  def Download(self, force_mirror=None):
    """Download upstream sources and verify integrity."""
    if self.IsGitUpstream():
      self.GitCloneToMirror()
      return

    archive = self.DownloadLocation()
    if not archive:
      return

    if force_mirror is None:
      force_mirror = os.environ.get('FORCE_MIRROR', False)
    self.DownloadArchive(force_mirror=force_mirror)

    if self.SHA1 is None:
      raise PkgFormatError('missing SHA1 attribute: %s' % self.info)

    util.VerifyHash(archive, self.SHA1)
    Log('verified: %s' % util.RelPath(archive))

  def Clean(self):
    pkg = self.PackageFile()
    Log('removing %s' % pkg)
    if os.path.exists(pkg):
      os.remove(pkg)

    stamp_dir = os.path.join(paths.STAMP_DIR, self.NAME)
    Log('removing %s' % stamp_dir)
    util.RemoveTree(stamp_dir)

  def Extract(self):
    """Extract the package archive into its build location.

    This method assumes the package has already been downloaded.
    """
    if self.IsGitUpstream():
      self.GitClone()
      return

    archive = self.DownloadLocation()
    if not archive:
      self.Log('Skipping extract; No upstream archive')
      return

    dest = self.GetBuildLocation()
    output_path, new_foldername = os.path.split(dest)
    util.Makedirs(output_path)

    # Check existing stamp file contents
    stamp_file = self.GetExtractStamp()
    stamp_contents = self.GetExtractStampContent()
    if os.path.exists(dest):
      if StampContentsMatch(stamp_file, stamp_contents):
        Log('Already up-to-date: %s' % util.RelPath(dest))
        return

      raise Error("Upstream archive or patch has changed.\n" +
                  "Please remove existing checkout and try again: '%s'" % dest)

    util.LogHeading('Extracting')
    util.Makedirs(paths.OUT_DIR)
    tmp_output_path = tempfile.mkdtemp(dir=paths.OUT_DIR)
    try:
      ExtractArchive(archive, tmp_output_path)
      src = os.path.join(tmp_output_path, new_foldername)
      if not os.path.isdir(src):
        raise Error('Archive contents not found: %s' % src)
      LogVerbose("renaming '%s' -> '%s'" % (src, dest))
      os.rename(src, dest)
    finally:
      util.RemoveTree(tmp_output_path)

    self.RemoveStamps()
    WriteStamp(stamp_file, stamp_contents)

  def RunCmd(self, cmd, **args):
    try:
      subprocess.check_call(cmd,
                            stdout=sys.stdout,
                            stderr=sys.stderr,
                            cwd=self.GetBuildLocation(), **args)
    except subprocess.CalledProcessError as e:
      raise Error(e)

  def Log(self, message):
    Log('%s: %s' % (message, self.InfoString()))

  def GetStampDir(self):
    return os.path.join(paths.STAMP_DIR, self.NAME)

  def RemoveStamps(self):
    util.RemoveTree(self.GetStampDir())

  def Patch(self):
    stamp_file = os.path.join(self.GetStampDir(), 'nacl_patch')
    src_dir = self.GetBuildLocation()
    if self.URL is None:
      return

    if os.path.exists(stamp_file):
      self.Log('Skipping patch step (cleaning source tree)')
      cmd = ['git', 'clean', '-f', '-d']
      if not util.log_level > util.LOG_INFO:
        cmd.append('-q')
      self.RunCmd(cmd)
      return

    util.LogHeading('Patching')
    InitGitRepo(src_dir)
    if os.path.exists(self.GetPatchFile()):
      LogVerbose('applying patch to: %s' % src_dir)
      cmd = ['patch', '-p1', '-g0', '--no-backup-if-mismatch']
      with open(self.GetPatchFile()) as f:
        self.RunCmd(cmd, stdin=f)
      self.RunCmd(['git', 'add', '.'])
      self.RunCmd(['git', 'commit', '-m', 'Apply naclports patch'])

    WriteStamp(stamp_file, '')

  def GetExtractStamp(self):
    return os.path.join(self.GetStampDir(), 'extract')

  def GetExtractStampContent(self):
    patch_file = self.GetPatchFile()
    if os.path.exists(patch_file):
      patch_sha = util.HashFile(self.GetPatchFile())
      return 'ARCHIVE_SHA1=%s PATCH_SHA1=%s\n' % (self.SHA1, patch_sha)
    else:
      return 'ARCHIVE_SHA1=%s\n' % self.SHA1

  def GetMirrorURL(self):
    return util.GS_MIRROR_URL + '/' + self.GetArchiveFilename()

  def CheckInstallable(self):
    if self.DISABLED:
      raise DisabledError('%s: package is disabled' % self.NAME)

    if self.LIBC is not None and self.LIBC != self.config.libc:
      raise DisabledError('%s: cannot be built with %s' %
                          (self.NAME, self.config.libc))

    if self.config.libc in self.DISABLED_LIBC:
      raise DisabledError('%s: cannot be built with %s' %
                          (self.NAME, self.config.libc))

    for disabled_toolchain in self.DISABLED_TOOLCHAIN:
      if '/' in disabled_toolchain:
        disabled_toolchain, arch = disabled_toolchain.split('/')
        if (self.config.arch == arch and
            self.config.toolchain == disabled_toolchain):
          raise DisabledError('%s: cannot be built with %s for %s' %
                              (self.NAME, self.config.toolchain, arch))
      else:
        if self.config.toolchain == disabled_toolchain:
          raise DisabledError('%s: cannot be built with %s' %
                              (self.NAME, self.config.toolchain))

    if self.config.arch in self.DISABLED_ARCH:
      raise DisabledError('%s: disabled for architecture: %s' %
                          (self.NAME, self.config.arch))

    if self.MIN_SDK_VERSION is not None:
      if not util.CheckSDKVersion(self.MIN_SDK_VERSION):
        raise DisabledError('%s: requires SDK version %s or above' %
                            (self.NAME, self.MIN_SDK_VERSION))

    if self.ARCH is not None:
      if self.config.arch not in self.ARCH:
        raise DisabledError('%s: disabled for architecture: %s' %
                            (self.NAME, self.config.arch))

    for conflicting_package in self.CONFLICTS:
      if util.IsInstalled(conflicting_package, self.config):
        raise PkgConflictError("%s: conflicts with installed package: %s" %
                               (self.NAME, conflicting_package))

    for dep in self.Dependencies():
      try:
        dep.CheckInstallable()
      except DisabledError as e:
        raise DisabledError('%s depends on %s; %s' % (self.NAME, dep.NAME, e))

  def Conflicts(self):
    """Yields the set of SourcePackages that directly conflict with this one"""
    for dep_name in self.CONFLICTS:
      yield CreatePackage(dep_name, self.config)

  def TransitiveConflicts(self):
    rtn = set(self.Conflicts())
    for dep in self.TransitiveDependencies():
      rtn |= set(dep.Conflicts())
    return rtn

  def Dependencies(self):
    """Yields the set of SourcePackages that this package directly depends on"""
    for dep_name in self.DEPENDS:
      yield CreatePackage(dep_name, self.config)

  def ReverseDependencies(self):
    """Yields the set of packages that depend directly on this one"""
    for pkg in SourcePackageIterator():
      if self.NAME in pkg.DEPENDS:
        yield pkg

  def TransitiveDependencies(self):
    """Yields the set of packages that this package transitively depends on"""
    deps = []
    for dep in self.Dependencies():
      for d in dep.TransitiveDependencies():
        if d not in deps:
          deps.append(d)
      if dep not in deps:
        deps.append(dep)
    return deps

  def CheckBuildable(self):
    self.CheckInstallable()

    if self.BUILD_OS is not None:
      if util.GetPlatform() != self.BUILD_OS:
        raise DisabledError('%s: can only be built on %s' %
                            (self.NAME, self.BUILD_OS))

  def GitCloneToMirror(self):
    """Clone the upstream git repo into a local mirror. """
    git_url, git_commit = self.URL.split('@', 2)

    # Clone upstream git repo into local mirror, or update the existing
    # mirror.
    git_mirror = git_url.split('://', 2)[1]
    git_mirror = git_mirror.replace('/', '_')
    mirror_dir = os.path.join(paths.CACHE_ROOT, git_mirror)
    if os.path.exists(mirror_dir):
      if RunGitCmd(mirror_dir, ['rev-parse', git_commit + '^{commit}'],
                   error_ok=True) != 0:
        Log('Updating git mirror: %s' % util.RelPath(mirror_dir))
        RunGitCmd(mirror_dir, ['remote', 'update', '--prune'])
    else:
      Log('Mirroring upstream git repo: %s' % self.URL)
      RunGitCmd(paths.CACHE_ROOT, ['clone', '--mirror', git_url, git_mirror])
    Log('git mirror up-to-date: %s' % util.RelPath(mirror_dir))
    return mirror_dir, git_commit

  def GitClone(self):
    """Create a clone of the upstream repo in the build directory.

    This operation will only require a network connection if the
    local git mirror is out-of-date."""
    stamp_file = self.GetExtractStamp()
    stamp_content = 'GITURL=%s' % self.URL
    patch_file = self.GetPatchFile()
    if os.path.exists(patch_file):
      patch_checksum = util.HashFile(patch_file)
      stamp_content += ' PATCH=%s' % patch_checksum

    stamp_content += '\n'

    dest = self.GetBuildLocation()
    if os.path.exists(self.GetBuildLocation()):
      if StampContentsMatch(stamp_file, stamp_content):
        return

      raise Error('Upstream archive or patch has changed.\n' +
                  "Please remove existing checkout and try again: '%s'" % dest)

    util.LogHeading('Cloning')
    # Ensure local mirror is up-to-date
    git_mirror, git_commit = self.GitCloneToMirror()
    # Clone from the local mirror.
    RunGitCmd(None, ['clone', git_mirror, dest])
    RunGitCmd(dest, ['reset', '--hard', git_commit])

    # Set the origing to the original URL so it is possible to push directly
    # from the build tree.
    RunGitCmd(dest, ['remote', 'set-url', 'origin', '${GIT_URL}'])

    self.RemoveStamps()
    WriteStamp(stamp_file, stamp_content)

  def DownloadArchive(self, force_mirror):
    """Download the archive to the local cache directory.

    Args:
      force_mirror: force downloading from mirror only.
    """
    filename = self.DownloadLocation()
    if not filename or os.path.exists(filename):
      return
    util.Makedirs(os.path.dirname(filename))

    # Always try the mirror URL first
    mirror_url = self.GetMirrorURL()
    try:
      util.DownloadFile(filename, mirror_url)
    except Error as e:
      if not force_mirror:
        # Fall back to the original upstream URL
        util.DownloadFile(filename, self.URL)
      else:
        raise e

  def UpdatePatch(self):
    if self.URL is None:
      return

    git_dir = self.GetBuildLocation()
    if not os.path.exists(git_dir):
      raise Error('Source directory not found: %s' % git_dir)

    try:
      diff = subprocess.check_output(['git', 'diff', 'upstream',
                                      '--no-ext-diff'],
                                     cwd=git_dir)
    except subprocess.CalledProcessError as e:
      raise Error('error running git in %s: %s' % (git_dir, str(e)))

    # Drop index lines for a more stable diff.
    diff = re.sub('\nindex [^\n]+\n', '\n', diff)

    # Drop binary files, as they don't work anyhow.
    diff = re.sub('diff [^\n]+\n'
                  '(new file [^\n]+\n)?'
                  '(deleted file mode [^\n]+\n)?'
                  'Binary files [^\n]+ differ\n', '', diff)

    # Always filter out config.sub changes
    diff_skip = ['*config.sub']

    # Add optional per-port skip list.
    diff_skip_file = os.path.join(self.root, 'diff_skip.txt')
    if os.path.exists(diff_skip_file):
      with open(diff_skip_file) as f:
        diff_skip += f.read().splitlines()

    new_diff = ''
    skipping = False
    for line in diff.splitlines():
      if line.startswith('diff --git a/'):
        filename = line[len('diff --git a/'):].split()[0]
        skipping = False
        for skip in diff_skip:
          if fnmatch.fnmatch(filename, skip):
            skipping = True
            break
      if not skipping:
        new_diff += line + '\n'
    diff = new_diff

    # Write back out the diff.
    patch_path = self.GetPatchFile()
    preexisting = os.path.exists(patch_path)

    if not diff:
      if preexisting:
        Log('removing patch file: %s' % util.RelPath(patch_path))
        os.remove(patch_path)
      else:
        Log('no patch required: %s' % util.RelPath(git_dir))
      return

    if preexisting:
      with open(patch_path) as f:
        if diff == f.read():
          Log('patch unchanged: %s' % util.RelPath(patch_path))
          return

    with open(patch_path, 'w') as f:
      f.write(diff)

    if preexisting:
      Log('created patch: %s' % util.RelPath(patch_path))
    else:
      Log('updated patch: %s' % util.RelPath(patch_path))


def SourcePackageIterator():
  """Iterator which yields a Package object for each naclports package."""
  ports_root = os.path.join(paths.NACLPORTS_ROOT, 'ports')
  for root, _, files in os.walk(ports_root):
    if 'pkg_info' in files:
      yield SourcePackage(root)


DEFAULT_LOCATIONS = ('ports', 'ports/python_modules')


def CreatePackage(package_name, config=None):
  """Create a Package object given a package name or path.

  Returns:
    Package object
  """
  if os.path.isdir(package_name):
    return SourcePackage(package_name, config)

  for subdir in DEFAULT_LOCATIONS:
    pkg_root = os.path.join(paths.NACLPORTS_ROOT, subdir, package_name)
    info = os.path.join(pkg_root, 'pkg_info')
    if os.path.exists(info):
      return SourcePackage(pkg_root, config)

  raise Error("Package not found: %s" % package_name)
