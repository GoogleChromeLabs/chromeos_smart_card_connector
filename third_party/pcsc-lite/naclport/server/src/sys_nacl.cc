/* Copyright (C) 1991-2014 Free Software Foundation, Inc.
   Copyright (C) 2016 Google Inc.

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

// This file is a replacement for the original sys_unix.c PC/SC-Lite internal
// implementation file.

#include <chrono>
#include <random>
#include <thread>

extern "C" int SYS_Sleep(int iTimeVal) {
  std::this_thread::sleep_for(std::chrono::seconds(iTimeVal));
  return 0;
}

extern "C" int SYS_USleep(int iTimeVal) {
  std::this_thread::sleep_for(std::chrono::microseconds(iTimeVal));
  return 0;
}

extern "C" int SYS_RandomInt(int fStart, int fEnd) {
  // Use C++11 pseudo-random number generator instead of C's rand(), since the
  // latter is broken in our PNaCl application (it gives).
  std::random_device random_device;
  unsigned random_value = random_device();
  if (fEnd == -1)
    return random_value;
  return fStart + (random_value % (fEnd - fStart + 1));
}

extern "C" void SYS_InitRandom() {}
