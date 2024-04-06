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

"""Runs ESLint on all relevant JavaScript files in the repository."""

import argparse
from internal import find_files_for_linting
import os
import os.path
import subprocess
import sys

def parse_command_line_args():
  parser = argparse.ArgumentParser(
      description='Runs ESLint on JavaScript files in the repository.')
  parser.add_argument('--base', type=str, default='main',
                      help='Git ref to diff against, or "none" if the whole '
                           'repository is to be checked (default: %(default)s)')
  parser.add_argument('--fix', action='store_true',
                      help='Specify to automatically fix errors when possible')
  return parser.parse_args()

def get_file_paths(args):
  return find_files_for_linting.find_files_for_linting(
      patterns=['*.js'], diff_base=args.base)

def run_linter(path, args):
  env_path = os.path.join(os.path.dirname(__file__), '../env/')
  command = ['npm', 'exec', '--prefix', env_path, 'eslint', '--', path,
             '--resolve-plugins-relative-to', env_path]
  if args.fix:
    command += ['--fix']
  env = os.environ.copy()
  # Force the older "eslintrc" config.
  env['ESLINT_USE_FLAT_CONFIG'] = 'false'
  return subprocess.call(command, env=env) == 0

def main():
  args = parse_command_line_args()
  paths = get_file_paths(args)
  return 0 if all([run_linter(path, args) for path in paths]) else 1

if __name__ == '__main__':
  sys.exit(main())
