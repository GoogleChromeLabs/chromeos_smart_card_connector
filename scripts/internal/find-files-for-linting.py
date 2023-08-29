#!/usr/bin/env python3

# Copyright 2023 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Returns paths to files to be linted/reformatted/etc.

It's the files that are manually maintained as part of this repository.
"""

import argparse
import os.path
import subprocess
import sys

# Manually maintained list of subtrees not intended for linting (mostly because
# they contain third-party code).
BLOCKLIST = [
  'third_party/ccid/src/',
  'third_party/closure-compiler-binary/src/',
  'third_party/closure-compiler/src/',
  'third_party/closure-library/src/',
  'third_party/googletest/src/',
  'third_party/libusb/src/',
  'third_party/nacl_sdk/nacl_sdk/',
  'third_party/pcsc-lite/src/',
  'third_party/webports/src/',
]

def is_suitable(path):
  return not any(is_parent_child(block, path) for block in BLOCKLIST)

def is_parent_child(parent, child):
  return os.path.commonpath([parent, child]) == os.path.commonpath([parent])

def parse_command_line_args():
  parser = argparse.ArgumentParser(
      description='Returns paths to files to be linted/reformatted/etc.')
  parser.add_argument('patterns', type=str, nargs='*',
                      help='return only files satisfying at least one pattern')
  parser.add_argument('--base', type=str, default='main',
                      help='Git ref to diff against, or "none" if the whole '
                           'repository is to be checked (default: %(default)s)')
  return parser.parse_args()

def get_git_command_prefix(base_arg):
  if base_arg == 'none':
    # "ls-files" is a fast way to list all files.
    # Note: "--full-name" is used to not depend on the work directory.
    return ['git', 'ls-files', '--full-name']
  # Use "diff" to list files between the given base and the current state.
  # Note: "d" in --diff-filter means excluding deleted files - they don't need
  # to be linted.
  return ['git', 'diff', '--name-only', '--diff-filter=d', base_arg]

def main():
  args = parse_command_line_args()
  # Note: ":/" is used to not depend on the work directory.
  command = (get_git_command_prefix(args.base) + ['--'] +
             [f':/{pattern}' for pattern in args.patterns])
  output = subprocess.check_output(command)
  for line in output.splitlines():
    path = line.strip().decode()
    if is_suitable(path):
      print(path)

if __name__ == '__main__':
  sys.exit(main())
