#!/usr/bin/env python
# Copyright (c) 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool to check if a glibc binary has a valid NEEDED order."""

import os
import re
import subprocess
import sys


def Check(filename):
  lines = subprocess.check_output(
      [os.environ.get('OBJDUMP', 'objdump'), '-p', filename]).splitlines()
  needed = [i for i in lines if 'NEEDED' in i]
  names = [re.match('[ ]+NEEDED[ ]+(.*)', i).group(1) for i in needed]
  pthread = -1
  nacl_io = -1
  nacl_spawn = -1
  for i, name in enumerate(names):
    if nacl_io < 0 and name == 'libnacl_io.so':
      nacl_io = i
    elif nacl_spawn < 0 and name == 'libnacl_spawn.so':
      nacl_spawn = i
    elif pthread < 0 and name.startswith('libpthread.so'):
      pthread = i
  if pthread != -1 and (pthread < nacl_io or pthread < nacl_spawn):
    print 'Executable %s has bad NEEDED order: %s' % (filename, ' '.join(names))
    return 1
  return 0


if __name__ == '__main__':
  sys.exit(Check(sys.argv[1]))
