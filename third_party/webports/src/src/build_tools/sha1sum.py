#!/usr/bin/env python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Usage:
 sha1sum.py <filename>

 will hash the filename in binary mode, generating a
 single line of output suitable for including in a
 pkg_info file. e.g.:

  SHA1=da39a3ee5e6b4b0d3255bfef95601890afd80709
"""

from __future__ import print_function

import hashlib
import os
import sys


def main(args):
  if len(args) != 1:
    sys.stderr.write("sha1sum.py: please specify a filename\n")
    sys.exit(-1)

  filename = args[0]
  if not os.path.exists(filename):
    sys.stderr.write("sha1sum.py: file not found: %s\n" % filename)
    sys.exit(-1)

  # open the file in binary mode & generate sha1 hash
  with open(filename, "rb") as f:
    h = hashlib.sha1()
    h.update(f.read())
    filehash = h.hexdigest()
    f.close()

  print("SHA1=%s" % filehash.lower())


if __name__ == '__main__':
  # all files hashed with success
  sys.exit(main(sys.argv[1:]))
