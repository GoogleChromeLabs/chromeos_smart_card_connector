// Copyright 2016 Google Inc.
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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFER_DESTINATION_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFER_DESTINATION_H_

#include <stdint.h>

#include <libusb.h>

#include "common/cpp/src/public/optional.h"

namespace google_smart_card {

// This structure uniquely represents a libusb/chrome.usb transfer destination.
//
// This structure is used for finding matches between transfers and transfer
// results (see the comments in the libusb_js_proxy.h header).
class UsbTransferDestination final {
 public:
  UsbTransferDestination();
  UsbTransferDestination(const UsbTransferDestination&) = default;
  UsbTransferDestination& operator=(const UsbTransferDestination&) = default;
  ~UsbTransferDestination();

  static UsbTransferDestination CreateForControlTransfer(
      int64_t js_device_handle,
      uint8_t request_type,
      uint8_t request,
      uint16_t value,
      uint16_t index);
  static UsbTransferDestination CreateForGenericTransfer(
      int64_t js_device_handle,
      uint8_t endpoint_address);

  bool IsInputDirection() const;

  bool operator<(const UsbTransferDestination& other) const;
  bool operator>(const UsbTransferDestination& other) const;
  bool operator==(const UsbTransferDestination& other) const;

 private:
  UsbTransferDestination(int64_t js_device_handle,
                         optional<uint8_t> endpoint_address,
                         optional<uint8_t> control_transfer_request_type,
                         optional<uint8_t> control_transfer_request,
                         optional<uint16_t> control_transfer_value,
                         optional<uint16_t> control_transfer_index);

  int Compare(const UsbTransferDestination& other) const;

  int64_t js_device_handle_;
  optional<uint8_t> endpoint_address_;
  optional<uint8_t> control_transfer_request_type_;
  optional<uint8_t> control_transfer_request_;
  optional<uint16_t> control_transfer_value_;
  optional<uint16_t> control_transfer_index_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFER_DESTINATION_H_
