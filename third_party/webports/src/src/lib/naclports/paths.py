# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACLPORTS_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

OUT_DIR = os.path.join(NACLPORTS_ROOT, 'out')
STAMP_DIR = os.path.join(OUT_DIR, 'stamp')
BUILD_ROOT = os.path.join(OUT_DIR, 'build')
PUBLISH_ROOT = os.path.join(OUT_DIR, 'publish')
TOOLS_DIR = os.path.join(NACLPORTS_ROOT, 'build_tools')
PACKAGES_ROOT = os.path.join(OUT_DIR, 'packages')
CACHE_ROOT = os.path.join(OUT_DIR, 'cache')
