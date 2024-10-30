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

#include "third_party/libusb/webport/src/usb_transfer_destination.h"

#include <stdint.h>

#include <tuple>

#include <libusb.h>

#include "common/cpp/src/public/logging/logging.h"

namespace google_smart_card {

UsbTransferDestination::UsbTransferDestination() = default;

UsbTransferDestination::~UsbTransferDestination() = default;

// static
UsbTransferDestination UsbTransferDestination::CreateForControlTransfer(
    int64_t js_device_handle,
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index) {
  return UsbTransferDestination(js_device_handle,
                                /*endpoint_address=*/{}, request_type, request,
                                value, index);
}

// static
UsbTransferDestination UsbTransferDestination::CreateForGenericTransfer(
    int64_t js_device_handle,
    uint8_t endpoint_address) {
  return UsbTransferDestination(
      js_device_handle, endpoint_address,
      /*control_transfer_request_type=*/{}, /*control_transfer_request=*/{},
      /*control_transfer_value=*/{}, /*control_transfer_index=*/{});
}

bool UsbTransferDestination::IsInputDirection() const {
  if (control_transfer_request_type_) {
    // For control transfers, the direction is encoded in the request type.
    return (*control_transfer_request_type_ & LIBUSB_ENDPOINT_DIR_MASK) ==
           LIBUSB_ENDPOINT_IN;
  }
  if (endpoint_address_) {
    // For all other transfer types, the direction is encoded in the endpoint
    // address.
    return (*endpoint_address_ & LIBUSB_ENDPOINT_DIR_MASK) ==
           LIBUSB_ENDPOINT_IN;
  }
  // It's invalid to call this function on a default-initialized instance.
  GOOGLE_SMART_CARD_NOTREACHED;
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
    int64_t js_device_handle,
    std::optional<uint8_t> endpoint_address,
    std::optional<uint8_t> control_transfer_request_type,
    std::optional<uint8_t> control_transfer_request,
    std::optional<uint16_t> control_transfer_value,
    std::optional<uint16_t> control_transfer_index)
    : js_device_handle_(js_device_handle),
      endpoint_address_(endpoint_address),
      control_transfer_request_type_(control_transfer_request_type),
      control_transfer_request_(control_transfer_request),
      control_transfer_value_(control_transfer_value),
      control_transfer_index_(control_transfer_index) {}

namespace {

template <typename T>
int CompareValues(const T& lhs, const T& rhs) {
  if (lhs < rhs)
    return -1;
  if (lhs > rhs)
    return 1;
  return 0;
}

}  // namespace

int UsbTransferDestination::Compare(const UsbTransferDestination& other) const {
  return CompareValues(
      std::tie(js_device_handle_, endpoint_address_,
               control_transfer_request_type_, control_transfer_request_,
               control_transfer_value_, control_transfer_index_),
      std::tie(other.js_device_handle_, other.endpoint_address_,
               other.control_transfer_request_type_,
               other.control_transfer_request_, other.control_transfer_value_,
               other.control_transfer_index_));
}

}  // namespace google_smart_card
