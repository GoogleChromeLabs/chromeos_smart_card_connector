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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_API_BRIDGE_INTERFACE_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_API_BRIDGE_INTERFACE_H_

#include <stdint.h>

#include <google_smart_card_common/requesting/request_result.h>

#include "chrome_usb/types.h"

namespace google_smart_card {

namespace chrome_usb {

// This interface represents a C++ bridge to the chrome.usb JavaScript API (see
// <https://developer.chrome.com/apps/usb>).
class ApiBridgeInterface {
 public:
  virtual ~ApiBridgeInterface() = default;

  virtual RequestResult<GetDevicesResult> GetDevices(
      const GetDevicesOptions& options) = 0;

  virtual RequestResult<GetUserSelectedDevicesResult> GetUserSelectedDevices(
      const GetUserSelectedDevicesOptions& options) = 0;

  virtual RequestResult<GetConfigurationsResult> GetConfigurations(
      const Device& device) = 0;

  virtual RequestResult<OpenDeviceResult> OpenDevice(const Device& device) = 0;

  virtual RequestResult<CloseDeviceResult> CloseDevice(
      const ConnectionHandle& connection_handle) = 0;

  virtual RequestResult<SetConfigurationResult> SetConfiguration(
      const ConnectionHandle& connection_handle,
      int64_t configuration_value) = 0;

  virtual RequestResult<GetConfigurationResult> GetConfiguration(
      const ConnectionHandle& connection_handle) = 0;

  virtual RequestResult<ListInterfacesResult> ListInterfaces(
      const ConnectionHandle& connection_handle) = 0;

  virtual RequestResult<ClaimInterfaceResult> ClaimInterface(
      const ConnectionHandle& connection_handle,
      int64_t interface_number) = 0;

  virtual RequestResult<ReleaseInterfaceResult> ReleaseInterface(
      const ConnectionHandle& connection_handle,
      int64_t interface_number) = 0;

  virtual void AsyncControlTransfer(const ConnectionHandle& connection_handle,
                                    const ControlTransferInfo& transfer_info,
                                    AsyncTransferCallback callback) = 0;

  virtual void AsyncBulkTransfer(const ConnectionHandle& connection_handle,
                                 const GenericTransferInfo& transfer_info,
                                 AsyncTransferCallback callback) = 0;

  virtual void AsyncInterruptTransfer(const ConnectionHandle& connection_handle,
                                      const GenericTransferInfo& transfer_info,
                                      AsyncTransferCallback callback) = 0;

  virtual RequestResult<ResetDeviceResult> ResetDevice(
      const ConnectionHandle& connection_handle) = 0;
};

}  // namespace chrome_usb

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_API_BRIDGE_INTERFACE_H_
