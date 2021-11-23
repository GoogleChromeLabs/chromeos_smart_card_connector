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

}  // namespace google_smart_card
