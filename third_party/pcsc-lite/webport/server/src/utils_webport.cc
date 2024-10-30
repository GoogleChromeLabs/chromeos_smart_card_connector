/* Copyright (C) 1991-2014 Free Software Foundation, Inc.
   Copyright (C) 2022 Google Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

// Custom implementation of some functions from the ../src/utils.h file.

extern "C" int CheckForOpenCT() {
  // This is a stub implementation that always returns success, which means
  // there's no OpenCT daemon (PC/SC-Lite exits when it detects it to be
  // present). We don't run the original code in a webport as it'd trigger
  // spurios warnings in JavaScript console about non-existing files.
  return 0;
}
