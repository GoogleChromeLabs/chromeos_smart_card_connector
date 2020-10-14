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

// This file contains definition of a structure UsbTransferDestination that
// uniquely represents a libusb/chrome.usb transfer destination.
//
// This structure is used for finding matches between transfers and transfer
// results (see the comments in the libusb_over_chrome_usb.h header).

#include "usb_transfer_destination.h"

#include <stdint.h>

#include <tuple>

namespace google_smart_card {

UsbTransferDestination::UsbTransferDestination() = default;

UsbTransferDestination::~UsbTransferDestination() = default;

// static
UsbTransferDestination
UsbTransferDestination::CreateFromChromeUsbControlTransfer(
    const chrome_usb::ConnectionHandle& connection_handle,
    const chrome_usb::ControlTransferInfo& transfer_info) {
  return UsbTransferDestination(
      connection_handle, transfer_info.direction, {}, transfer_info.recipient,
      transfer_info.request_type, transfer_info.request, transfer_info.value,
      transfer_info.index);
}

// static
UsbTransferDestination
UsbTransferDestination::CreateFromChromeUsbGenericTransfer(
    const chrome_usb::ConnectionHandle& connection_handle,
    const chrome_usb::GenericTransferInfo& transfer_info) {
  return UsbTransferDestination(connection_handle, transfer_info.direction,
                                transfer_info.endpoint, {}, {}, {}, {}, {});
}

bool UsbTransferDestination::IsInputDirection() const {
  return direction_ == chrome_usb::Direction::kIn;
}

bool UsbTransferDestination::operator<(
    const UsbTransferDestination& other) const {
  return Compare(other) < 0;
}

bool UsbTransferDestination::operator>(
    const UsbTransferDestination& other) const {
  return Compare(other) > 0;
}

bool UsbTransferDestination::operator==(
    const UsbTransferDestination& other) const {
  return Compare(other) == 0;
}

UsbTransferDestination::UsbTransferDestination(
    const chrome_usb::ConnectionHandle& connection_handle,
    const chrome_usb::Direction& direction, optional<int64_t> endpoint,
    optional<chrome_usb::ControlTransferInfoRecipient>
        control_transfer_recipient,
    optional<chrome_usb::ControlTransferInfoRequestType>
        control_transfer_request_type,
    optional<int64_t> control_transfer_request,
    optional<int64_t> control_transfer_value,
    optional<int64_t> control_transfer_index)
    : connection_handle_(connection_handle),
      direction_(direction),
      endpoint_(endpoint),
      control_transfer_recipient_(control_transfer_recipient),
      control_transfer_request_type_(control_transfer_request_type),
      control_transfer_request_(control_transfer_request),
      control_transfer_value_(control_transfer_value),
      control_transfer_index_(control_transfer_index) {}

namespace {

template <typename T>
int CompareValues(const T& lhs, const T& rhs) {
  if (lhs < rhs) return -1;
  if (lhs > rhs) return 1;
  return 0;
}

}  // namespace

int UsbTransferDestination::Compare(const UsbTransferDestination& other) const {
  return CompareValues(
      std::tie(connection_handle_.handle, connection_handle_.vendor_id,
               connection_handle_.product_id, direction_, endpoint_,
               control_transfer_recipient_, control_transfer_request_type_,
               control_transfer_request_, control_transfer_value_,
               control_transfer_index_),
      std::tie(
          other.connection_handle_.handle, other.connection_handle_.vendor_id,
          other.connection_handle_.product_id, other.direction_,
          other.endpoint_, other.control_transfer_recipient_,
          other.control_transfer_request_type_, other.control_transfer_request_,
          other.control_transfer_value_, other.control_transfer_index_));
}

}  // namespace google_smart_card
