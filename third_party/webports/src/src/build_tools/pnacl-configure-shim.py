#!/usr/bin/env python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Autoconf builds lots of small executables.
This wreaks havock with pnacl's slow -O2 build time.
Additionally linking nacl_io + ppapi_simple slows things down even more.

This script is injected for CC to speed up configure by:
- When configuring:
  - Drop -O2 and -O3
  - Add -O0
  - Drop nacl_spawn + nacl_io + cli_main and their dependencies.
"""

import subprocess
import sys

cmd = sys.argv[1:]
configuring = 'conftest.c' in cmd or 'conftest.pexe' in cmd

DROP_FLAGS = {
    '-O2',
    '-O3',
    '-Dmain=nacl_main',
    '-Dmain=SDL_main',
    '-lnacl_io',
    '-lnacl_spawn',
    '-lppapi_simple',
    '-lppapi_cpp',
    '-lppapi',
    '-lcli_main',
    '-lSDLmain',
}

if configuring:
  cmd = [i for i in cmd if i not in DROP_FLAGS]
  cmd += ['-O0']

sys.exit(subprocess.call(cmd))
