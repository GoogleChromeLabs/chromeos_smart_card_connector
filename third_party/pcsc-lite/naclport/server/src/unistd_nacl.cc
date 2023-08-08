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

// Implementation of some functions from the <unistd.h> header are missing under
// PNaCl SDK, so the definitions necessary for pcsc-lite compilation are
// provided here.

#include <unistd.h>

#include "common/cpp/src/public/logging/logging.h"

unsigned alarm(unsigned /*seconds*/) {
  // This is a stub implementation that should be never invoked.
  GOOGLE_SMART_CARD_NOTREACHED;
}
