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

#include "chrome_usb/api_bridge.h"

#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/construction.h>

namespace google_smart_card {

namespace chrome_usb {

ApiBridge::ApiBridge(std::unique_ptr<Requester> requester)
    : requester_(std::move(requester)), remote_call_adaptor_(requester_.get()) {
  GOOGLE_SMART_CARD_CHECK(requester_);
}

ApiBridge::~ApiBridge() = default;

void ApiBridge::Detach() {
  requester_->Detach();
}

RequestResult<GetDevicesResult> ApiBridge::GetDevices(
    const GetDevicesOptions& options) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("getDevices", options);
  GetDevicesResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result, &result.devices);
}

RequestResult<GetUserSelectedDevicesResult> ApiBridge::GetUserSelectedDevices(
    const GetUserSelectedDevicesOptions& options) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("getUserSelectedDevices", options);
  GetUserSelectedDevicesResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result, &result.devices);
}

RequestResult<GetConfigurationsResult> ApiBridge::GetConfigurations(
    const Device& device) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("getConfigurations", device);
  GetConfigurationsResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result, &result.configurations);
}

RequestResult<OpenDeviceResult> ApiBridge::OpenDevice(const Device& device) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("openDevice", device);
  OpenDeviceResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result, &result.connection_handle);
}

RequestResult<CloseDeviceResult> ApiBridge::CloseDevice(
    const ConnectionHandle& connection_handle) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("closeDevice", connection_handle);
  CloseDeviceResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result);
}

RequestResult<SetConfigurationResult> ApiBridge::SetConfiguration(
    const ConnectionHandle& connection_handle,
    int64_t configuration_value) {
  GenericRequestResult generic_request_result = remote_call_adaptor_.SyncCall(
      "setConfiguration", connection_handle, configuration_value);
  SetConfigurationResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result);
}

RequestResult<GetConfigurationResult> ApiBridge::GetConfiguration(
    const ConnectionHandle& connection_handle) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("getConfiguration", connection_handle);
  GetConfigurationResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result, &result.configuration);
}

RequestResult<ListInterfacesResult> ApiBridge::ListInterfaces(
    const ConnectionHandle& connection_handle) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("listInterfaces", connection_handle);
  ListInterfacesResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result, &result.descriptors);
}

RequestResult<ClaimInterfaceResult> ApiBridge::ClaimInterface(
    const ConnectionHandle& connection_handle,
    int64_t interface_number) {
  GenericRequestResult generic_request_result = remote_call_adaptor_.SyncCall(
      "claimInterface", connection_handle, interface_number);
  ClaimInterfaceResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result);
}

RequestResult<ReleaseInterfaceResult> ApiBridge::ReleaseInterface(
    const ConnectionHandle& connection_handle,
    int64_t interface_number) {
  GenericRequestResult generic_request_result = remote_call_adaptor_.SyncCall(
      "releaseInterface", connection_handle, interface_number);
  ReleaseInterfaceResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result);
}

namespace {

GenericAsyncRequestCallback WrapAsyncTransferCallback(
    AsyncTransferCallback callback) {
  return [callback](GenericRequestResult generic_request_result) {
    TransferResult result;
    return callback(RemoteCallAdaptor::ConvertResultPayload(
        std::move(generic_request_result), &result, &result.result_info));
  };
}

}  // namespace

void ApiBridge::AsyncControlTransfer(const ConnectionHandle& connection_handle,
                                     const ControlTransferInfo& transfer_info,
                                     AsyncTransferCallback callback) {
  remote_call_adaptor_.AsyncCall(WrapAsyncTransferCallback(callback),
                                 "controlTransfer", connection_handle,
                                 transfer_info);
}

void ApiBridge::AsyncBulkTransfer(const ConnectionHandle& connection_handle,
                                  const GenericTransferInfo& transfer_info,
                                  AsyncTransferCallback callback) {
  remote_call_adaptor_.AsyncCall(WrapAsyncTransferCallback(callback),
                                 "bulkTransfer", connection_handle,
                                 transfer_info);
}

void ApiBridge::AsyncInterruptTransfer(
    const ConnectionHandle& connection_handle,
    const GenericTransferInfo& transfer_info,
    AsyncTransferCallback callback) {
  remote_call_adaptor_.AsyncCall(WrapAsyncTransferCallback(callback),
                                 "interruptTransfer", connection_handle,
                                 transfer_info);
}

RequestResult<ResetDeviceResult> ApiBridge::ResetDevice(
    const ConnectionHandle& connection_handle) {
  GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("resetDevice", connection_handle);
  ResetDeviceResult result;
  return RemoteCallAdaptor::ConvertResultPayload(
      std::move(generic_request_result), &result, &result.reset_success);
}

}  // namespace chrome_usb

}  // namespace google_smart_card
