#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys


def Main(argv):
  # Check the number of arguments.
  if len(argv) < 2:
    sys.stderr.write('Usage: %s <template> [KEY=VALUE]*\n' % argv[0])
    return 1
  # Decode substitutions.
  substitutions = {}
  for arg in argv[2:]:
    key, value = arg.split('=', 1)
    substitutions[key] = value
  # Load the template.
  with open(argv[1], 'rb') as fh:
    data = fh.read()
  # Emit template with substituions.
  sys.stdout.write(data % substitutions)
  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
