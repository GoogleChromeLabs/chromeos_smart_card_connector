// Copyright 2024 Google Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef GOOGLE_SMART_CARD_LIBUSB_CONSTANTS_H_
#define GOOGLE_SMART_CARD_LIBUSB_CONSTANTS_H_

#include <stdint.h>

namespace google_smart_card {

// Stub out the device bus number with this value initially (as the JS API does
// not provide means of retrieving it). We also allow the client code override
// this to a different value if needed.
constexpr uint8_t kDefaultUsbBusNumber = 1;

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_LIBUSB_CONSTANTS_H_
