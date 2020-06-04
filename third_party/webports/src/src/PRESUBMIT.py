# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Documentation on PRESUBMIT.py can be found at:
# http://www.chromium.org/developers/how-tos/depottools/presubmit-scripts

import os
import subprocess

PYTHON = 'build_tools/python_wrapper'

_EXCLUDED_PATHS = (
    # patch_configure.py contains long lines embedded in multi-line
    # strings.
    r"^build_tools[\\\/]patch_configure.py",
    # Many of the files in glibc-compat come from the other sources such as
    # newlib and as such do not contain our copyright header.
    r"^ports[\/\\]glibc-compat[\/\\]include[\/\\]err\.h",
    r"^ports[\/\\]glibc-compat[\/\\]include[\/\\]fts\.h",
    r"^ports[\/\\]glibc-compat[\/\\]src[\/\\]err\.c",
    r"^ports[\/\\]glibc-compat[\/\\]src[\/\\]libc-symbols\.h",
    r"^ports[\/\\]glibc-compat[\/\\]src[\/\\]dirfd.c",
    r"^ports[\/\\]glibc-compat[\/\\]src[\/\\]flock.c",
    r"^ports[\/\\]glibc-compat[\/\\]src[\/\\]fts.c",
    r"^ports[\/\\]glibc-compat[\/\\]src[\/\\]herror.c",
    r"^ports[\/\\]glibc-compat[\/\\]src[\/\\]timegm.c",
)

def RunPylint(input_api, output_api):
  output = []
  canned = input_api.canned_checks
  disabled_warnings = [
    'W0613',  # Unused argument
  ]
  black_list = list(input_api.DEFAULT_BLACK_LIST) + [
    r'ports[\/\\]ipython-ppapi[\/\\]kernel\.py',
  ]
  output.extend(canned.RunPylint(input_api, output_api, black_list=black_list,
                disabled_warnings=disabled_warnings, extra_paths_list=['lib']))
  return output



def RunCommand(name, cmd, input_api, output_api):
  try:
    subprocess.check_call(cmd)
  except subprocess.CalledProcessError:
    message = '%s failed.' % name
    return [output_api.PresubmitError(message)]
  return []


def RunPythonCommand(cmd, input_api, output_api):
  return RunCommand(cmd[0], [PYTHON] + cmd, input_api, output_api)


def CheckPartioning(input_api, output_api):
  return RunPythonCommand(['build_tools/partition.py', '--check'],
                          input_api,
                          output_api)


def CheckDeps(input_api, output_api):
  return RunPythonCommand(['build_tools/check_deps.py'],
                          input_api,
                          output_api)

def CheckMirror(input_api, output_api):
  return RunPythonCommand(['build_tools/update_mirror.py', '--check'],
                          input_api,
                          output_api)


def RunUnittests(input_api, output_api):
  return RunCommand('unittests', ['make', 'test'], input_api, output_api)


# This check was copied from the chromium version.
# TODO(sbc): should we add this to canned_checks?
def CheckAuthorizedAuthor(input_api, output_api):
  """Verify the author's email address is in AUTHORS.
  """
  import fnmatch

  author = input_api.change.author_email
  if not author:
    input_api.logging.info('No author, skipping AUTHOR check')
    return []
  authors_path = input_api.os_path.join(
      input_api.PresubmitLocalPath(), 'AUTHORS')
  valid_authors = (
      input_api.re.match(r'[^#]+\s+\<(.+?)\>\s*$', line)
      for line in open(authors_path))
  valid_authors = [item.group(1).lower() for item in valid_authors if item]
  if not any(fnmatch.fnmatch(author.lower(), valid) for valid in valid_authors):
    input_api.logging.info('Valid authors are %s', ', '.join(valid_authors))
    return [output_api.PresubmitPromptWarning(
        ('%s is not in AUTHORS file. If you are a new contributor, please visit'
        '\n'
        'http://www.chromium.org/developers/contributing-code and read the '
        '"Legal" section.\n') % author)]
  return []


def CheckChangeOnUpload(input_api, output_api):
  report = []
  report.extend(CheckAuthorizedAuthor(input_api, output_api))
  report.extend(RunPylint(input_api, output_api))
  report.extend(RunUnittests(input_api, output_api))
  report.extend(CheckDeps(input_api, output_api))
  report.extend(input_api.canned_checks.PanProjectChecks(
      input_api, output_api, project_name='Native Client',
      excluded_paths=_EXCLUDED_PATHS))
  return report


def CheckChangeOnCommit(input_api, output_api):
  report = []
  report.extend(CheckChangeOnUpload(input_api, output_api))
  report.extend(CheckMirror(input_api, output_api))
  report.extend(CheckPartioning(input_api, output_api))
  report.extend(input_api.canned_checks.CheckTreeIsOpen(
      input_api, output_api,
      json_url='http://naclports-status.appspot.com/current?format=json'))
  return report


TRYBOTS = [
    'naclports-linux-glibc-0',
    'naclports-linux-glibc-1',
    'naclports-linux-glibc-2',
    'naclports-linux-glibc-3',
    'naclports-linux-glibc-4',
    'naclports-linux-glibc-5',
    'naclports-linux-pnacl-0',
    'naclports-linux-pnacl-1',
    'naclports-linux-pnacl-2',
    'naclports-linux-pnacl-3',
    'naclports-linux-pnacl-4',
    'naclports-linux-pnacl-5',
    'naclports-linux-clang-0',
    'naclports-linux-clang-1',
    'naclports-linux-clang-2',
    'naclports-linux-clang-3',
    'naclports-linux-clang-4',
    'naclports-linux-clang-5',
    'naclports-linux-emscripten-0',
]


def GetPreferredTryMasters(_, change):
  return {
    'tryserver.nacl': { t: set(['defaulttests']) for t in TRYBOTS },
  }
