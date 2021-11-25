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

#include <libusb.h>

namespace google_smart_card {

namespace {

// TODO(#429): Delete this converter once the C++ code gets fully abstracted
// away from chrome.usb.
uint8_t GetLibusbRequestType(
    const chrome_usb::ControlTransferInfo& chrome_usb_control_transfer_info) {
  uint8_t request_type = 0;
  switch (chrome_usb_control_transfer_info.request_type) {
    case chrome_usb::ControlTransferInfoRequestType::kStandard:
      request_type |= LIBUSB_REQUEST_TYPE_STANDARD;
      break;
    case chrome_usb::ControlTransferInfoRequestType::kClass:
      request_type |= LIBUSB_REQUEST_TYPE_CLASS;
      break;
    case chrome_usb::ControlTransferInfoRequestType::kVendor:
      request_type |= LIBUSB_REQUEST_TYPE_VENDOR;
      break;
    case chrome_usb::ControlTransferInfoRequestType::kReserved:
      request_type |= LIBUSB_REQUEST_TYPE_RESERVED;
      break;
  }
  switch (chrome_usb_control_transfer_info.direction) {
    case chrome_usb::Direction::kIn:
      request_type |= LIBUSB_ENDPOINT_IN;
      break;
    case chrome_usb::Direction::kOut:
      request_type |= LIBUSB_ENDPOINT_OUT;
      break;
  }
  switch (chrome_usb_control_transfer_info.recipient) {
    case chrome_usb::ControlTransferInfoRecipient::kDevice:
      request_type |= LIBUSB_RECIPIENT_DEVICE;
      break;
    case chrome_usb::ControlTransferInfoRecipient::kInterface:
      request_type |= LIBUSB_RECIPIENT_INTERFACE;
      break;
    case chrome_usb::ControlTransferInfoRecipient::kEndpoint:
      request_type |= LIBUSB_RECIPIENT_ENDPOINT;
      break;
    case chrome_usb::ControlTransferInfoRecipient::kOther:
      request_type |= LIBUSB_RECIPIENT_OTHER;
      break;
  }
  return request_type;
}

}  // namespace

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

// static
UsbTransferDestination
UsbTransferDestination::CreateFromChromeUsbControlTransfer(
    const chrome_usb::ConnectionHandle& connection_handle,
    const chrome_usb::ControlTransferInfo& transfer_info) {
  return UsbTransferDestination(
      connection_handle.handle,
      /*endpoint_address=*/{}, GetLibusbRequestType(transfer_info),
      transfer_info.request, transfer_info.value, transfer_info.index);
}

// static
UsbTransferDestination
UsbTransferDestination::CreateFromChromeUsbGenericTransfer(
    const chrome_usb::ConnectionHandle& connection_handle,
    const chrome_usb::GenericTransferInfo& transfer_info) {
  return UsbTransferDestination(
      connection_handle.handle, transfer_info.endpoint,
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
    optional<uint8_t> endpoint_address,
    optional<uint8_t> control_transfer_request_type,
    optional<uint8_t> control_transfer_request,
    optional<uint16_t> control_transfer_value,
    optional<uint16_t> control_transfer_index)
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
