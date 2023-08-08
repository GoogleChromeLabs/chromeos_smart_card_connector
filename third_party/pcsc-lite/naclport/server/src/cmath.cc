// Copyright (C) 1997-2013 Free Software Foundation, Inc.
// Copyright (C) 2016 Google Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.
//
// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

// Implementation of some functions from the <cmath> header are missing under
// PNaCl SDK, so the definitions necessary for pcsc-lite compilation are
// provided here.

#include <cmath>

#include "common/cpp/src/public/logging/logging.h"

extern "C" double nexttoward(double /*x*/, long double /*y*/) {
  // FIXME(emaxx): Investigate why does the PNaCl standard library causes the
  // linkage error without this stub implementation.
  GOOGLE_SMART_CARD_NOTREACHED;
}
