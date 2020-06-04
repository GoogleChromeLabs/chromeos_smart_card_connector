# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Error(Exception):
  """General error used for all naclports-specific errors."""
  pass


class PkgFormatError(Error):
  """Error raised when package file is not valid."""
  pass


class DisabledError(Error):
  """Error raised when a package is cannot be built because it is disabled
  for the current configuration.
  """
  pass
