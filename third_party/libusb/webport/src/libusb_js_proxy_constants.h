// Copyright 2023 Google Inc.
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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_JS_PROXY_CONSTANTS_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_JS_PROXY_CONSTANTS_H_

namespace google_smart_card {

// The requester name that's used by the C++ libusb_js_proxy code.
//
// The JavaScript side has a "request receiver" that receives and handles the
// calls by triggering the corresponding JavaScript USB API functionality (see
// libusb-to-js-api-adaptor.js).
extern const char kLibusbJsProxyRequesterName[];

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_JS_PROXY_CONSTANTS_H_
