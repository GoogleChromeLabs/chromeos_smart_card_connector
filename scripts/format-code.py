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

"""Runs clang-format on all relevant C++/JavaScript files in the repository."""

import argparse
import os.path
import subprocess
import sys

FILE_MASKS = ['*.cc', '*.h', '*.js']
FORMAT_OPTIONS = [
  '--style=Chromium',
  # TODO(emaxx): Consider reenabling this, now that we don't use <> includes for
  # the headers maintained in this project.
  '--sort-includes=false',
]
CHECK_ONLY_OPTIONS = ['--dry-run', '-Werror']

def parse_command_line_args():
  parser = argparse.ArgumentParser(
      description='Runs clang-format on C++/JS files in the repository.')
  parser.add_argument(
      '--base', type=str, default='main',
      help='Git ref to diff against, or "none" if the whole repository is to '
           'be reformatted (default: %(default)s)')
  parser.add_argument(
      '--check-only', action='store_true',
      help='When specified, files are only checked for formatting, but no '
           'edits are made automatically. Non-zero exit code will indicate at '
           'least one formatting issue.')
  parser.add_argument(
      '--llvm-version', type=int,
      help='LLVM version to use, i.e., "10" to use "clang-format-10". If '
           'not specified, unqualified "clang-format" is used.')
  return parser.parse_args()

def get_file_paths(args):
  command = [os.path.join(os.path.dirname(__file__),
                          'internal/find-files-for-linting.py'),
             '--base', args.base] + FILE_MASKS
  return [line.strip().decode() for line
          in subprocess.check_output(command).splitlines()]

def run_clang_format(path, args):
  program = (f'clang-format-{args.llvm_version}' if args.llvm_version else
             'clang-format')
  command = [program, '-i'] + FORMAT_OPTIONS
  if args.check_only:
    command += CHECK_ONLY_OPTIONS
  command += [path]
  print(f'Running {program}...')
  return subprocess.call(command) == 0

def main():
  args = parse_command_line_args()
  paths = get_file_paths(args)
  return 0 if all([run_clang_format(path, args) for path in paths]) else 1

if __name__ == '__main__':
  sys.exit(main())
