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

#include <google_smart_card_common/optional.h>

#include "chrome_usb/types.h"

namespace google_smart_card {

// This structure uniquely represents a libusb/chrome.usb transfer destination.
//
// This structure is used for finding matches between transfers and transfer
// results (see the comments in the libusb_over_chrome_usb.h header).
class UsbTransferDestination final {
 public:
  UsbTransferDestination();
  UsbTransferDestination(const UsbTransferDestination&) = default;
  UsbTransferDestination& operator=(const UsbTransferDestination&) = default;
  ~UsbTransferDestination();

  static UsbTransferDestination CreateFromChromeUsbControlTransfer(
      const chrome_usb::ConnectionHandle& connection_handle,
      const chrome_usb::ControlTransferInfo& transfer_info);

  static UsbTransferDestination CreateFromChromeUsbGenericTransfer(
      const chrome_usb::ConnectionHandle& connection_handle,
      const chrome_usb::GenericTransferInfo& transfer_info);

  bool IsInputDirection() const;

  bool operator<(const UsbTransferDestination& other) const;
  bool operator>(const UsbTransferDestination& other) const;
  bool operator==(const UsbTransferDestination& other) const;

 private:
  UsbTransferDestination(const chrome_usb::ConnectionHandle& connection_handle,
                         const chrome_usb::Direction& direction,
                         optional<int64_t> endpoint,
                         optional<chrome_usb::ControlTransferInfoRecipient>
                             control_transfer_recipient,
                         optional<chrome_usb::ControlTransferInfoRequestType>
                             control_transfer_request_type,
                         optional<int64_t> control_transfer_request,
                         optional<int64_t> control_transfer_value,
                         optional<int64_t> control_transfer_index);

  int Compare(const UsbTransferDestination& other) const;

  chrome_usb::ConnectionHandle connection_handle_;
  chrome_usb::Direction direction_;
  optional<int64_t> endpoint_;
  optional<chrome_usb::ControlTransferInfoRecipient>
      control_transfer_recipient_;
  optional<chrome_usb::ControlTransferInfoRequestType>
      control_transfer_request_type_;
  optional<int64_t> control_transfer_request_;
  optional<int64_t> control_transfer_value_;
  optional<int64_t> control_transfer_index_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFER_DESTINATION_H_
