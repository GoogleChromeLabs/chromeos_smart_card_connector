// Copyright 2021 Google Inc.
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

#include "libusb_js_proxy_data_model.h"

#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

// Define conversions of every type to/from Value, so that the generic
// implementation for sending/receiving them to/from the JS side works:

template <>
StructValueDescriptor<LibusbJsDevice>::Description
StructValueDescriptor<LibusbJsDevice>::GetDescription() {
  // Note: Strings passed to WithField() below must match the ones in
  // libusb-proxy-data-model.js.
  return Describe("LibusbJsDevice")
      .WithField(&LibusbJsDevice::device_id, "deviceId")
      .WithField(&LibusbJsDevice::vendor_id, "vendorId")
      .WithField(&LibusbJsDevice::product_id, "productId")
      .WithField(&LibusbJsDevice::version, "version")
      .WithField(&LibusbJsDevice::product_name, "productName")
      .WithField(&LibusbJsDevice::manufacturer_name, "manufacturerName")
      .WithField(&LibusbJsDevice::serial_number, "serialNumber");
}

template <>
EnumValueDescriptor<LibusbJsDirection>::Description
EnumValueDescriptor<LibusbJsDirection>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the ones in
  // libusb-proxy-data-model.js.
  return Describe("LibusbJsDirection")
      .WithItem(LibusbJsDirection::kIn, "in")
      .WithItem(LibusbJsDirection::kOut, "out");
}

template <>
EnumValueDescriptor<LibusbJsEndpointType>::Description
EnumValueDescriptor<LibusbJsEndpointType>::GetDescription() {
  // Note: Strings passed to WithItem() below must match the ones in
  // libusb-proxy-data-model.js.
  return Describe("LibusbJsEndpointType")
      .WithItem(LibusbJsEndpointType::kBulk, "bulk")
      .WithItem(LibusbJsEndpointType::kControl, "control")
      .WithItem(LibusbJsEndpointType::kInterrupt, "interrupt")
      .WithItem(LibusbJsEndpointType::kIsochronous, "isochronous");
}

template <>
StructValueDescriptor<LibusbJsEndpointDescriptor>::Description
StructValueDescriptor<LibusbJsEndpointDescriptor>::GetDescription() {
  // Note: Strings passed to WithField() below must match the ones in
  // libusb-proxy-data-model.js.
  return Describe("LibusbJsEndpointDescriptor")
      .WithField(&LibusbJsEndpointDescriptor::endpoint_address,
                 "endpointAddress")
      .WithField(&LibusbJsEndpointDescriptor::direction, "direction")
      .WithField(&LibusbJsEndpointDescriptor::type, "type")
      .WithField(&LibusbJsEndpointDescriptor::extra_data, "extraData")
      .WithField(&LibusbJsEndpointDescriptor::max_packet_size, "maxPacketSize");
}

template <>
StructValueDescriptor<LibusbJsInterfaceDescriptor>::Description
StructValueDescriptor<LibusbJsInterfaceDescriptor>::GetDescription() {
  // Note: Strings passed to WithField() below must match the ones in
  // libusb-proxy-data-model.js.
  return Describe("LibusbJsInterfaceDescriptor")
      .WithField(&LibusbJsInterfaceDescriptor::interface_number,
                 "interfaceNumber")
      .WithField(&LibusbJsInterfaceDescriptor::interface_class,
                 "interfaceClass")
      .WithField(&LibusbJsInterfaceDescriptor::interface_subclass,
                 "interfaceSubclass")
      .WithField(&LibusbJsInterfaceDescriptor::interface_protocol,
                 "interfaceProtocol")
      .WithField(&LibusbJsInterfaceDescriptor::extra_data, "extraData")
      .WithField(&LibusbJsInterfaceDescriptor::endpoints, "endpoints");
}

template <>
StructValueDescriptor<LibusbJsConfigurationDescriptor>::Description
StructValueDescriptor<LibusbJsConfigurationDescriptor>::GetDescription() {
  // Note: Strings passed to WithField() below must match the ones in
  // libusb-proxy-data-model.js.
  return Describe("LibusbJsConfigurationDescriptor")
      .WithField(&LibusbJsConfigurationDescriptor::active, "active")
      .WithField(&LibusbJsConfigurationDescriptor::configuration_value,
                 "configurationValue")
      .WithField(&LibusbJsConfigurationDescriptor::extra_data, "extraData")
      .WithField(&LibusbJsConfigurationDescriptor::interfaces, "interfaces");
}

}  // namespace google_smart_card
