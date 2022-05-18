#!/usr/bin/env python3

# Copyright 2022 Google Inc. All Rights Reserved.
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

"""Parses the output of "llvm-cov report" and reformats it."""

import sys

def split_columns(line):
  """Splits the tokens separated by two or more spaces.

  This is the format of the "table" that's emitted by llvm-cov report.
  """
  return [item.strip() for item in line.split('  ') if item]

def parse_llvm_cov_report_from_stdin():
  """Interprets the whole stdin as a "table" from llvm-cov report.

  Returns:
    (column_names, rows)
  """
  # Parse the table's header.
  header_line = next(sys.stdin)
  column_names = split_columns(header_line)
  if len(column_names) <= 1 or column_names[0] != 'Filename':
    raise RuntimeError(f'Unexpected header: ${header_line}')
  # Parse the table's contents.
  column_names = split_columns(header_line)
  rows = []
  for line in sys.stdin:
    cells = split_columns(line)
    if len(cells) <= 1:
      # Empty line, or line with separators ("---..."), or line with a subtitle
      # (e.g., "Files which contain no functions:").
      continue
    if len(cells) != len(column_names):
      raise RuntimeError(f'Unexpected row line: ${line}')
    rows.append(cells)
  return column_names, rows

def get_total_line_coverage(column_names, rows):
  lines_coverage_column = column_names.index('Lines') + 2
  if column_names[lines_coverage_column] != 'Cover':
    raise RuntimeError(f'Failed to find line coverage column: ${column_names}')
  if rows[-1][0] != 'TOTAL':
    raise RuntimeError(f'Could not find totals row: ${rows[-1]}')
  return rows[-1][lines_coverage_column]

def main():
  column_names, rows = parse_llvm_cov_report_from_stdin()
  print(get_total_line_coverage(column_names, rows))

if __name__ == '__main__':
  sys.exit(main())
