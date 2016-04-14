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

// This file provides definitions that provide the C++ bridge to the chrome.usb
// JavaScript API (see <https://developer.chrome.com/apps/usb>).

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_API_BRIDGE_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_API_BRIDGE_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/requesting/async_request.h>
#include <google_smart_card_common/requesting/remote_call_adaptor.h>
#include <google_smart_card_common/requesting/requester.h>
#include <google_smart_card_common/requesting/request_result.h>

#include "chrome_usb/types.h"

namespace google_smart_card {

namespace chrome_usb {

constexpr char kApiBridgeRequesterName[] = "libusb_chrome_usb";

struct GetDevicesResult {
  std::vector<Device> devices;
};

struct GetUserSelectedDevicesResult {
  std::vector<Device> devices;
};

struct GetConfigurationsResult {
  std::vector<ConfigDescriptor> configurations;
};

struct OpenDeviceResult {
  ConnectionHandle connection_handle;
};

struct CloseDeviceResult {};

struct SetConfigurationResult {};

struct GetConfigurationResult {
  ConfigDescriptor configuration;
};

struct ListInterfacesResult {
  std::vector<InterfaceDescriptor> descriptors;
};

struct ClaimInterfaceResult {};

struct ReleaseInterfaceResult {};

struct TransferResult {
  TransferResultInfo result_info;
};

using AsyncTransferCallback = std::function<
    void(RequestResult<TransferResult>)>;

struct ResetDeviceResult {
  bool reset_success;
};

// This class provides a C++ bridge to the chrome.usb JavaScript API (see
// <https://developer.chrome.com/apps/usb>).
//
// The integration with the JavaScript API is done by performing the requests of
// some special form to the JavaScript side. On the JavaScript side, the handler
// of these requests will call the corresponding chrome.usb API methods (see the
// chrome-usb-backend.js and the chrome-usb-handler.js files).
class ApiBridge final {
 public:
  explicit ApiBridge(std::unique_ptr<Requester> requester);

  void Detach();

  RequestResult<GetDevicesResult> GetDevices(const GetDevicesOptions& options);

  RequestResult<GetUserSelectedDevicesResult> GetUserSelectedDevices(
      const GetUserSelectedDevicesOptions& options);

  RequestResult<GetConfigurationsResult> GetConfigurations(
      const Device& device);

  RequestResult<OpenDeviceResult> OpenDevice(const Device& device);

  RequestResult<CloseDeviceResult> CloseDevice(
      const ConnectionHandle& connection_handle);

  RequestResult<SetConfigurationResult> SetConfiguration(
      const ConnectionHandle& connection_handle, int64_t configuration_value);

  RequestResult<GetConfigurationResult> GetConfiguration(
      const ConnectionHandle& connection_handle);

  RequestResult<ListInterfacesResult> ListInterfaces(
      const ConnectionHandle& connection_handle);

  RequestResult<ClaimInterfaceResult> ClaimInterface(
      const ConnectionHandle& connection_handle, int64_t interface_number);

  RequestResult<ReleaseInterfaceResult> ReleaseInterface(
      const ConnectionHandle& connection_handle, int64_t interface_number);

  RequestResult<TransferResult> ControlTransfer(
      const ConnectionHandle& connection_handle,
      const ControlTransferInfo& transfer_info);

  RequestResult<TransferResult> BulkTransfer(
      const ConnectionHandle& connection_handle,
      const GenericTransferInfo& transfer_info);

  RequestResult<TransferResult> InterruptTransfer(
      const ConnectionHandle& connection_handle,
      const GenericTransferInfo& transfer_info);

  void AsyncControlTransfer(
      const ConnectionHandle& connection_handle,
      const ControlTransferInfo& transfer_info,
      AsyncTransferCallback callback,
      AsyncRequest* async_request);

  void AsyncBulkTransfer(
      const ConnectionHandle& connection_handle,
      const GenericTransferInfo& transfer_info,
      AsyncTransferCallback callback,
      AsyncRequest* async_request);

  void AsyncInterruptTransfer(
      const ConnectionHandle& connection_handle,
      const GenericTransferInfo& transfer_info,
      AsyncTransferCallback callback,
      AsyncRequest* async_request);

  RequestResult<ResetDeviceResult> ResetDevice(
      const ConnectionHandle& connection_handle);

 private:
  std::unique_ptr<Requester> requester_;
  RemoteCallAdaptor remote_call_adaptor_;
};

}  // namespace chrome_usb

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_CHROME_USB_API_BRIDGE_H_
