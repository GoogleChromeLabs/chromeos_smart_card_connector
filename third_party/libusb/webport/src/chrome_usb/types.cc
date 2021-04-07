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

#include "chrome_usb/types.h"

#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

using chrome_usb::ConfigDescriptor;
using chrome_usb::ConnectionHandle;
using chrome_usb::ControlTransferInfo;
using chrome_usb::ControlTransferInfoRecipient;
using chrome_usb::ControlTransferInfoRequestType;
using chrome_usb::Device;
using chrome_usb::DeviceFilter;
using chrome_usb::Direction;
using chrome_usb::EndpointDescriptor;
using chrome_usb::EndpointDescriptorSynchronization;
using chrome_usb::EndpointDescriptorType;
using chrome_usb::EndpointDescriptorUsage;
using chrome_usb::GenericTransferInfo;
using chrome_usb::GetDevicesOptions;
using chrome_usb::GetUserSelectedDevicesOptions;
using chrome_usb::InterfaceDescriptor;
using chrome_usb::TransferResultInfo;

bool Device::operator==(const Device& other) const {
  return device == other.device && vendor_id == other.vendor_id &&
         product_id == other.product_id && version == other.version &&
         product_name == other.product_name &&
         manufacturer_name == other.manufacturer_name &&
         serial_number == other.serial_number;
}

bool ConnectionHandle::operator==(const ConnectionHandle& other) const {
  return handle == other.handle && vendor_id == other.vendor_id &&
         product_id == other.product_id;
}

bool ControlTransferInfo::operator==(const ControlTransferInfo& other) const {
  return direction == other.direction && recipient == other.recipient &&
         request_type == other.request_type && request == other.request &&
         value == other.value && index == other.index &&
         length == other.length && data == other.data &&
         timeout == other.timeout;
}

template <>
EnumValueDescriptor<Direction>::Description
EnumValueDescriptor<Direction>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.usb API.
  return Describe("chrome_usb::Direction")
      .WithItem(Direction::kIn, "in")
      .WithItem(Direction::kOut, "out");
}

template <>
StructValueDescriptor<Device>::Description
StructValueDescriptor<Device>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::Device")
      .WithField(&Device::device, "device")
      .WithField(&Device::vendor_id, "vendorId")
      .WithField(&Device::product_id, "productId")
      .WithField(&Device::version, "version")
      .WithField(&Device::product_name, "productName")
      .WithField(&Device::manufacturer_name, "manufacturerName")
      .WithField(&Device::serial_number, "serialNumber");
}

template <>
StructValueDescriptor<ConnectionHandle>::Description
StructValueDescriptor<ConnectionHandle>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::ConnectionHandle")
      .WithField(&ConnectionHandle::handle, "handle")
      .WithField(&ConnectionHandle::vendor_id, "vendorId")
      .WithField(&ConnectionHandle::product_id, "productId");
}

template <>
EnumValueDescriptor<EndpointDescriptorType>::Description
EnumValueDescriptor<EndpointDescriptorType>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.usb API.
  return Describe("chrome_usb::EndpointDescriptorType")
      .WithItem(EndpointDescriptorType::kControl, "control")
      .WithItem(EndpointDescriptorType::kInterrupt, "interrupt")
      .WithItem(EndpointDescriptorType::kIsochronous, "isochronous")
      .WithItem(EndpointDescriptorType::kBulk, "bulk");
}

template <>
EnumValueDescriptor<EndpointDescriptorSynchronization>::Description
EnumValueDescriptor<EndpointDescriptorSynchronization>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.usb API.
  return Describe("chrome_usb::EndpointDescriptorSynchronization")
      .WithItem(EndpointDescriptorSynchronization::kAsynchronous,
                "asynchronous")
      .WithItem(EndpointDescriptorSynchronization::kAdaptive, "adaptive")
      .WithItem(EndpointDescriptorSynchronization::kSynchronous, "synchronous");
}

template <>
EnumValueDescriptor<EndpointDescriptorUsage>::Description
EnumValueDescriptor<EndpointDescriptorUsage>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.usb API.
  return Describe("chrome_usb::EndpointDescriptorUsage")
      .WithItem(EndpointDescriptorUsage::kData, "data")
      .WithItem(EndpointDescriptorUsage::kFeedback, "feedback")
      .WithItem(EndpointDescriptorUsage::kExplicitFeedback, "explicitFeedback")
      .WithItem(EndpointDescriptorUsage::kPeriodic, "periodic")
      .WithItem(EndpointDescriptorUsage::kNotification, "notification");
}

template <>
StructValueDescriptor<EndpointDescriptor>::Description
StructValueDescriptor<EndpointDescriptor>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::EndpointDescriptor")
      .WithField(&EndpointDescriptor::address, "address")
      .WithField(&EndpointDescriptor::type, "type")
      .WithField(&EndpointDescriptor::direction, "direction")
      .WithField(&EndpointDescriptor::maximum_packet_size, "maximumPacketSize")
      .WithField(&EndpointDescriptor::synchronization, "synchronization")
      .WithField(&EndpointDescriptor::usage, "usage")
      .WithField(&EndpointDescriptor::polling_interval, "pollingInterval")
      .WithField(&EndpointDescriptor::extra_data, "extra_data");
}

template <>
StructValueDescriptor<InterfaceDescriptor>::Description
StructValueDescriptor<InterfaceDescriptor>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::InterfaceDescriptor")
      .WithField(&InterfaceDescriptor::interface_number, "interfaceNumber")
      .WithField(&InterfaceDescriptor::alternate_setting, "alternateSetting")
      .WithField(&InterfaceDescriptor::interface_class, "interfaceClass")
      .WithField(&InterfaceDescriptor::interface_subclass, "interfaceSubclass")
      .WithField(&InterfaceDescriptor::interface_protocol, "interfaceProtocol")
      .WithField(&InterfaceDescriptor::description, "description")
      .WithField(&InterfaceDescriptor::endpoints, "endpoints")
      .WithField(&InterfaceDescriptor::extra_data, "extra_data");
}

template <>
StructValueDescriptor<ConfigDescriptor>::Description
StructValueDescriptor<ConfigDescriptor>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::ConfigDescriptor")
      .WithField(&ConfigDescriptor::active, "active")
      .WithField(&ConfigDescriptor::configuration_value, "configurationValue")
      .WithField(&ConfigDescriptor::description, "description")
      .WithField(&ConfigDescriptor::self_powered, "selfPowered")
      .WithField(&ConfigDescriptor::remote_wakeup, "remoteWakeup")
      .WithField(&ConfigDescriptor::max_power, "maxPower")
      .WithField(&ConfigDescriptor::interfaces, "interfaces")
      .WithField(&ConfigDescriptor::extra_data, "extra_data");
}

template <>
StructValueDescriptor<GenericTransferInfo>::Description
StructValueDescriptor<GenericTransferInfo>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::GenericTransferInfo")
      .WithField(&GenericTransferInfo::direction, "direction")
      .WithField(&GenericTransferInfo::endpoint, "endpoint")
      .WithField(&GenericTransferInfo::length, "length")
      .WithField(&GenericTransferInfo::data, "data")
      .WithField(&GenericTransferInfo::timeout, "timeout");
}

template <>
EnumValueDescriptor<ControlTransferInfoRecipient>::Description
EnumValueDescriptor<ControlTransferInfoRecipient>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.usb API.
  return Describe("chrome_usb::ControlTransferInfoRecipient")
      .WithItem(ControlTransferInfoRecipient::kDevice, "device")
      .WithItem(ControlTransferInfoRecipient::kInterface, "interface")
      .WithItem(ControlTransferInfoRecipient::kEndpoint, "endpoint")
      .WithItem(ControlTransferInfoRecipient::kOther, "other");
}

template <>
EnumValueDescriptor<ControlTransferInfoRequestType>::Description
EnumValueDescriptor<ControlTransferInfoRequestType>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the enum names in the
  // chrome.usb API.
  return Describe("chrome_usb::ControlTransferInfoRequestType")
      .WithItem(ControlTransferInfoRequestType::kStandard, "standard")
      .WithItem(ControlTransferInfoRequestType::kClass, "class")
      .WithItem(ControlTransferInfoRequestType::kVendor, "vendor")
      .WithItem(ControlTransferInfoRequestType::kReserved, "reserved");
}

template <>
StructValueDescriptor<ControlTransferInfo>::Description
StructValueDescriptor<ControlTransferInfo>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::ControlTransferInfo")
      .WithField(&ControlTransferInfo::direction, "direction")
      .WithField(&ControlTransferInfo::recipient, "recipient")
      .WithField(&ControlTransferInfo::request_type, "requestType")
      .WithField(&ControlTransferInfo::request, "request")
      .WithField(&ControlTransferInfo::value, "value")
      .WithField(&ControlTransferInfo::index, "index")
      .WithField(&ControlTransferInfo::length, "length")
      .WithField(&ControlTransferInfo::data, "data")
      .WithField(&ControlTransferInfo::timeout, "timeout");
}

template <>
StructValueDescriptor<TransferResultInfo>::Description
StructValueDescriptor<TransferResultInfo>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::TransferResultInfo")
      .WithField(&TransferResultInfo::result_code, "resultCode")
      .WithField(&TransferResultInfo::data, "data");
}

template <>
StructValueDescriptor<DeviceFilter>::Description
StructValueDescriptor<DeviceFilter>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::DeviceFilter")
      .WithField(&DeviceFilter::vendor_id, "vendorId")
      .WithField(&DeviceFilter::product_id, "productId")
      .WithField(&DeviceFilter::interface_class, "interfaceClass")
      .WithField(&DeviceFilter::interface_subclass, "interfaceSubclass")
      .WithField(&DeviceFilter::interface_protocol, "interfaceProtocol");
}

template <>
StructValueDescriptor<GetDevicesOptions>::Description
StructValueDescriptor<GetDevicesOptions>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::GetDevicesOptions")
      .WithField(&GetDevicesOptions::filters, "filters");
}

template <>
StructValueDescriptor<GetUserSelectedDevicesOptions>::Description
StructValueDescriptor<GetUserSelectedDevicesOptions>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names in
  // the chrome.usb API.
  return Describe("chrome_usb::GetUserSelectedDevicesOptions")
      .WithField(&GetUserSelectedDevicesOptions::filters, "filters");
}

}  // namespace google_smart_card
