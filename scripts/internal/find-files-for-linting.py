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
import subprocess
import sys

def parse_command_line_args():
  parser = argparse.ArgumentParser(
      description='Returns paths to files to be linted/reformatted/etc.')
  parser.add_argument('patterns', type=str, nargs='*',
                      help='return only files satisfying at least one pattern')
  return parser.parse_args()

def main():
  args = parse_command_line_args()
  # Note: "d" in --diff-filter means excluding deleted files - they don't need
  # to be linted.
  return subprocess.check_call(
      ['git', 'diff', '--name-only', '--diff-filter=d', 'main', '--'] +
          args.patterns)

if __name__ == '__main__':
  sys.exit(main())
