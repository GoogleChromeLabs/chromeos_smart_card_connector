// Copyright 2022 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "smart_card_connector_app/src/testing_smart_card_simulation.h"

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_debug_dumping.h>

#include "common/cpp/src/public/value_builder.h"
#include "third_party/libusb/webport/src/libusb_js_proxy_data_model.h"

namespace google_smart_card {

using DeviceType = TestingSmartCardSimulation::DeviceType;

const char* TestingSmartCardSimulation::kRequesterName = "libusb";

namespace {

LibusbJsDevice MakeLibusbJsDevice(
    const TestingSmartCardSimulation::Device& device) {
  LibusbJsDevice js_device;
  js_device.device_id = device.id;
  switch (device.type) {
    case DeviceType::kGemaltoPcTwinReader:
      // Numbers are for a real device.
      js_device.vendor_id = 0x08E6;
      js_device.product_id = 0x3437;
      return js_device;
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

std::vector<LibusbJsConfigurationDescriptor>
MakeLibusbJsConfigurationDescriptors(DeviceType device_type) {
  switch (device_type) {
    case DeviceType::kGemaltoPcTwinReader: {
      // Values are taken from a real device.

      LibusbJsEndpointDescriptor endpoint1;
      endpoint1.endpoint_address = 1;
      endpoint1.direction = LibusbJsDirection::kOut;
      endpoint1.type = LibusbJsEndpointType::kBulk;
      endpoint1.max_packet_size = 64;

      LibusbJsEndpointDescriptor endpoint2;
      endpoint2.endpoint_address = 0x82;
      endpoint2.direction = LibusbJsDirection::kIn;
      endpoint2.type = LibusbJsEndpointType::kBulk;
      endpoint2.max_packet_size = 64;

      LibusbJsEndpointDescriptor endpoint3;
      endpoint3.endpoint_address = 0x83;
      endpoint3.direction = LibusbJsDirection::kIn;
      endpoint3.type = LibusbJsEndpointType::kInterrupt;
      endpoint3.max_packet_size = 8;

      LibusbJsInterfaceDescriptor interface;
      interface.interface_number = 0;
      interface.interface_class = 0xB;
      interface.interface_subclass = 0;
      interface.interface_protocol = 0;
      interface.extra_data = std::vector<uint8_t>(
          {0x36, 0x21, 0x01, 0x01, 0x00, 0x07, 0x03, 0x00, 0x00, 0x00, 0xC0,
           0x12, 0x00, 0x00, 0xC0, 0x12, 0x00, 0x00, 0x00, 0x67, 0x32, 0x00,
           0x00, 0xCE, 0x99, 0x0C, 0x00, 0x35, 0xFE, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x01, 0x00,
           0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01});
      interface.endpoints.push_back(std::move(endpoint1));
      interface.endpoints.push_back(std::move(endpoint2));
      interface.endpoints.push_back(std::move(endpoint3));

      LibusbJsConfigurationDescriptor config;
      config.active = true;
      config.configuration_value = 1;
      config.interfaces.push_back(std::move(interface));

      return {config};
    }
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

bool DeviceInterfaceExists(DeviceType device_type, int64_t interface_number) {
  for (const auto& config : MakeLibusbJsConfigurationDescriptors(device_type)) {
    for (const auto& interface : config.interfaces) {
      if (interface.interface_number == interface_number)
        return true;
    }
  }
  return false;
}

bool DeviceEndpointExists(DeviceType device_type, uint8_t endpoint_address) {
  for (const auto& config : MakeLibusbJsConfigurationDescriptors(device_type)) {
    for (const auto& interface : config.interfaces) {
      for (const auto& endpoint : interface.endpoints) {
        if (endpoint.endpoint_address == endpoint_address)
          return true;
      }
    }
  }
  return false;
}

// Builds a fake response to the GET_DATA_RATES control transfer.
std::vector<uint8_t> MakeGetDataRatesResponse(DeviceType device_type) {
  switch (device_type) {
    case DeviceType::kGemaltoPcTwinReader: {
      // Values are taken from a real device.
      return {0x67, 0x32, 0x00, 0x00, 0xCE, 0x64, 0x00, 0x00, 0x9D, 0xC9, 0x00,
              0x00, 0x3A, 0x93, 0x01, 0x00, 0x74, 0x26, 0x03, 0x00, 0xE7, 0x4C,
              0x06, 0x00, 0xCE, 0x99, 0x0C, 0x00, 0xD7, 0x5C, 0x02, 0x00, 0x11,
              0xF0, 0x03, 0x00, 0x34, 0x43, 0x00, 0x00, 0x69, 0x86, 0x00, 0x00,
              0xD1, 0x0C, 0x01, 0x00, 0xA2, 0x19, 0x02, 0x00, 0x45, 0x33, 0x04,
              0x00, 0x8A, 0x66, 0x08, 0x00, 0x0B, 0xA0, 0x02, 0x00, 0x73, 0x30,
              0x00, 0x00, 0xE6, 0x60, 0x00, 0x00, 0xCC, 0xC1, 0x00, 0x00, 0x99,
              0x83, 0x01, 0x00, 0x32, 0x07, 0x03, 0x00, 0x63, 0x0E, 0x06, 0x00,
              0xB3, 0x22, 0x01, 0x00, 0x7F, 0xE4, 0x01, 0x00, 0x06, 0x50, 0x01,
              0x00, 0x36, 0x97, 0x00, 0x00, 0x04, 0xFC, 0x00, 0x00, 0x53, 0x28,
              0x00, 0x00, 0xA5, 0x50, 0x00, 0x00, 0x4A, 0xA1, 0x00, 0x00, 0x95,
              0x42, 0x01, 0x00, 0x29, 0x85, 0x02, 0x00, 0xF8, 0x78, 0x00, 0x00,
              0x3E, 0x49, 0x00, 0x00, 0x7C, 0x92, 0x00, 0x00, 0xF8, 0x24, 0x01,
              0x00, 0xF0, 0x49, 0x02, 0x00, 0xE0, 0x93, 0x04, 0x00, 0xC0, 0x27,
              0x09, 0x00, 0x74, 0xB7, 0x01, 0x00, 0x6C, 0xDC, 0x02, 0x00, 0xD4,
              0x30, 0x00, 0x00, 0xA8, 0x61, 0x00, 0x00, 0x50, 0xC3, 0x00, 0x00,
              0xA0, 0x86, 0x01, 0x00, 0x40, 0x0D, 0x03, 0x00, 0x80, 0x1A, 0x06,
              0x00, 0x48, 0xE8, 0x01, 0x00, 0xBA, 0xDB, 0x00, 0x00, 0x36, 0x6E,
              0x01, 0x00, 0x24, 0xF4, 0x00, 0x00, 0xDD, 0x6D, 0x00, 0x00, 0x1B,
              0xB7, 0x00, 0x00};
    }
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

// Builds a fake RDR_to_PC_SlotStatus message.
std::vector<uint8_t> MakeSlotStatusTransferReply(uint8_t sequence_number) {
  // Currently, the status always says "no ICC present".
  const uint8_t status = 0x2;
  return {0x81,   0x00, 0x00, 0x00, 0x00, 0x00, sequence_number,
          status, 0x00, 0x00};
}

// Builds a fake RDR_to_PC_Escape message.
std::vector<uint8_t> MakeEscapeTransferReply(uint8_t sequence_number) {
  // Currently, the status always says "escape command failed" and "no ICC
  // present".
  const uint8_t status = 0x42;
  return {0x83,   0x00, 0x00, 0x00, 0x00, 0x00, sequence_number,
          status, 0x0A, 0x00};
}

}  // namespace

TestingSmartCardSimulation::TestingSmartCardSimulation(
    TypedMessageRouter* typed_message_router)
    : typed_message_router_(typed_message_router) {}

TestingSmartCardSimulation::~TestingSmartCardSimulation() = default;

void TestingSmartCardSimulation::OnRequestToJs(Value request_payload,
                                               optional<RequestId> request_id) {
  // Make the debug dump in advance, before we know whether we need to crash,
  // because we can't dump the value after std::move()'ing it.
  const std::string payload_debug_dump = DebugDumpValueFull(request_payload);

  RemoteCallRequestPayload remote_call =
      ConvertFromValueOrDie<RemoteCallRequestPayload>(
          std::move(request_payload));
  if (remote_call.function_name == "listDevices") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.empty());
    OnListDevicesCalled(*request_id);
    return;
  }
  if (remote_call.function_name == "getConfigurations") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 1);
    OnGetConfigurationsCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(), *request_id);
    return;
  }
  if (remote_call.function_name == "openDeviceHandle") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 1);
    OnOpenDeviceHandleCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(), *request_id);
    return;
  }
  if (remote_call.function_name == "closeDeviceHandle") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 2);
    OnCloseDeviceHandleCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(), *request_id);
    return;
  }
  if (remote_call.function_name == "claimInterface") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    OnClaimInterfaceCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        /*interface_number=*/remote_call.arguments[2].GetInteger(),
        *request_id);
    return;
  }
  if (remote_call.function_name == "releaseInterface") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    OnReleaseInterfaceCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        /*interface_number=*/remote_call.arguments[2].GetInteger(),
        *request_id);
    return;
  }
  if (remote_call.function_name == "controlTransfer") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    OnControlTransferCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        ConvertFromValueOrDie<LibusbJsControlTransferParameters>(
            std::move(remote_call.arguments[2])),
        *request_id);
    return;
  }
  if (remote_call.function_name == "bulkTransfer") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    OnBulkTransferCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        ConvertFromValueOrDie<LibusbJsGenericTransferParameters>(
            std::move(remote_call.arguments[2])),
        *request_id);
    return;
  }
  if (remote_call.function_name == "interruptTransfer") {
    GOOGLE_SMART_CARD_CHECK(remote_call.arguments.size() == 3);
    OnInterruptTransferCalled(
        /*device_id=*/remote_call.arguments[0].GetInteger(),
        /*device_handle=*/remote_call.arguments[1].GetInteger(),
        ConvertFromValueOrDie<LibusbJsGenericTransferParameters>(
            std::move(remote_call.arguments[2])),
        *request_id);
    return;
  }
  GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected request: " << payload_debug_dump;
}

void TestingSmartCardSimulation::SetDevices(
    const std::vector<Device>& devices) {
  std::unique_lock<std::mutex> lock(mutex_);

  std::vector<DeviceState> old_device_states = device_states_;
  device_states_.clear();
  for (const auto& device : devices) {
    DeviceState state;
    // If this device was already present, reuse its state.
    for (const auto& old_state : old_device_states) {
      if (old_state.device.id == device.id)
        state = old_state;
    }
    // Update the `device` field unconditionally, to reflect any changes the
    // caller might've provided in it.
    state.device = device;
    device_states_.push_back(state);
  }
}

void TestingSmartCardSimulation::PostFakeJsResult(RequestId request_id,
                                                  Value result) {
  ResponseMessageData response_data;
  response_data.request_id = request_id;
  response_data.payload = ArrayValueBuilder().Add(std::move(result)).Get();
  PostResponseMessage(std::move(response_data));
}

void TestingSmartCardSimulation::PostFakeJsError(RequestId request_id,
                                                 const std::string& error) {
  ResponseMessageData response_data;
  response_data.request_id = request_id;
  response_data.error_message = error;
  PostResponseMessage(std::move(response_data));
}

void TestingSmartCardSimulation::PostResponseMessage(
    ResponseMessageData response_data) {
  TypedMessage response;
  response.type = GetResponseMessageType(kRequesterName);
  response.data = ConvertToValueOrDie(std::move(response_data));

  Value reply = ConvertToValueOrDie(std::move(response));

  std::string error_message;
  if (!typed_message_router_->OnMessageReceived(std::move(reply),
                                                &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Dispatching fake JS reply failed: "
                                << error_message;
  }
}

TestingSmartCardSimulation::DeviceState*
TestingSmartCardSimulation::FindDeviceStateByIdLocked(int64_t device_id) {
  for (auto& device_state : device_states_) {
    if (device_state.device.id == device_id)
      return &device_state;
  }
  return nullptr;
}

TestingSmartCardSimulation::DeviceState*
TestingSmartCardSimulation::FindDeviceStateByIdAndHandleLocked(
    int64_t device_id,
    int64_t device_handle) {
  DeviceState* device_state = FindDeviceStateByIdLocked(device_id);
  if (device_state && device_state->opened_device_handle == device_handle)
    return device_state;
  return nullptr;
}

void TestingSmartCardSimulation::OnListDevicesCalled(RequestId request_id) {
  std::vector<std::unique_ptr<Value>> libusb_js_devices;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto& device_state : device_states_) {
      libusb_js_devices.push_back(MakeUnique<Value>(
          ConvertToValueOrDie(MakeLibusbJsDevice(device_state.device))));
    }
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  PostFakeJsResult(request_id, Value(std::move(libusb_js_devices)));
}

void TestingSmartCardSimulation::OnGetConfigurationsCalled(
    int64_t device_id,
    RequestId request_id) {
  optional<DeviceType> device_type;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state = FindDeviceStateByIdLocked(device_id);
    if (device_state)
      device_type = device_state->device.type;
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!device_type) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  PostFakeJsResult(
      request_id,
      ConvertToValueOrDie(MakeLibusbJsConfigurationDescriptors(*device_type)));
}

void TestingSmartCardSimulation::OnOpenDeviceHandleCalled(
    int64_t device_id,
    RequestId request_id) {
  optional<int> handle;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state = FindDeviceStateByIdLocked(device_id);
    if (device_state) {
      GOOGLE_SMART_CARD_CHECK(!device_state->opened_device_handle);
      device_state->opened_device_handle = next_free_device_handle_;
      handle = next_free_device_handle_;
      ++next_free_device_handle_;
    }
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!handle) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  PostFakeJsResult(request_id, Value(*handle));
}

void TestingSmartCardSimulation::OnCloseDeviceHandleCalled(
    int64_t device_id,
    int64_t device_handle,
    RequestId request_id) {
  bool device_found = false;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state =
        FindDeviceStateByIdAndHandleLocked(device_id, device_handle);
    if (device_state) {
      GOOGLE_SMART_CARD_CHECK(device_state->opened_device_handle);
      device_found = true;
      device_state->opened_device_handle = {};
    }
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!device_found) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  PostFakeJsResult(request_id, Value());
}

void TestingSmartCardSimulation::OnClaimInterfaceCalled(
    int64_t device_id,
    int64_t device_handle,
    int64_t interface_number,
    RequestId request_id) {
  bool device_found = false;
  bool interface_claimed = false;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state =
        FindDeviceStateByIdAndHandleLocked(device_id, device_handle);
    if (device_state) {
      device_found = true;
      GOOGLE_SMART_CARD_CHECK(
          DeviceInterfaceExists(device_state->device.type, interface_number));
      if (!device_state->claimed_interfaces.count(interface_number)) {
        device_state->claimed_interfaces.insert(interface_number);
        interface_claimed = true;
      }
    }
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!device_found) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  if (!interface_claimed) {
    PostFakeJsError(request_id, "Already claimed interface");
    return;
  }
  PostFakeJsResult(request_id, Value());
}

void TestingSmartCardSimulation::OnReleaseInterfaceCalled(
    int64_t device_id,
    int64_t device_handle,
    int64_t interface_number,
    RequestId request_id) {
  bool device_found = false;
  bool interface_released = false;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state =
        FindDeviceStateByIdAndHandleLocked(device_id, device_handle);
    if (device_state) {
      device_found = true;
      GOOGLE_SMART_CARD_CHECK(
          DeviceInterfaceExists(device_state->device.type, interface_number));
      if (device_state->claimed_interfaces.count(interface_number)) {
        device_state->claimed_interfaces.erase(interface_number);
        interface_released = true;
      }
    }
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!device_found) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  if (!interface_released) {
    PostFakeJsError(request_id, "Unclaimed interface");
    return;
  }
  PostFakeJsResult(request_id, Value());
}

void TestingSmartCardSimulation::OnControlTransferCalled(
    int64_t device_id,
    int64_t device_handle,
    LibusbJsControlTransferParameters params,
    RequestId request_id) {
  optional<DeviceType> device_type;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state =
        FindDeviceStateByIdAndHandleLocked(device_id, device_handle);
    if (device_state)
      device_type = device_state->device.type;
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!device_type) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  if (params.request_type == LibusbJsTransferRequestType::kClass &&
      params.recipient == LibusbJsTransferRecipient::kInterface &&
      params.request == 3) {
    // GET_DATA_RATES request to the reader.
    GOOGLE_SMART_CARD_CHECK(!params.data_to_send);
    LibusbJsTransferResult result;
    result.received_data = MakeGetDataRatesResponse(*device_type);
    GOOGLE_SMART_CARD_CHECK(*params.length_to_receive >=
                            result.received_data->size());
    PostFakeJsResult(request_id, ConvertToValueOrDie(std::move(result)));
    return;
  }
  PostFakeJsError(request_id, "Unknown control command");
}

void TestingSmartCardSimulation::OnBulkTransferCalled(
    int64_t device_id,
    int64_t device_handle,
    LibusbJsGenericTransferParameters params,
    RequestId request_id) {
  bool device_found = false;
  optional<LibusbJsTransferResult> result;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state =
        FindDeviceStateByIdAndHandleLocked(device_id, device_handle);
    if (device_state) {
      device_found = true;
      GOOGLE_SMART_CARD_CHECK(DeviceEndpointExists(device_state->device.type,
                                                   params.endpoint_address));
      if (params.data_to_send) {
        result =
            HandleOutputBulkTransferLocked(*params.data_to_send, *device_state);
      } else {
        result = HandleInputBulkTransferLocked(*params.length_to_receive,
                                               *device_state);
      }
    }
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!device_found) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  if (!result) {
    PostFakeJsError(request_id, "Transfer failed");
    return;
  }
  PostFakeJsResult(request_id, ConvertToValueOrDie(std::move(*result)));
}

void TestingSmartCardSimulation::OnInterruptTransferCalled(
    int64_t device_id,
    int64_t device_handle,
    LibusbJsGenericTransferParameters params,
    RequestId request_id) {
  optional<DeviceType> device_type;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    DeviceState* device_state =
        FindDeviceStateByIdAndHandleLocked(device_id, device_handle);
    if (device_state)
      device_type = device_state->device.type;
    GOOGLE_SMART_CARD_CHECK(DeviceEndpointExists(device_state->device.type,
                                                 params.endpoint_address));
  }

  // Reply outside mutex, to prevent potential deadlocks in case the libusb
  // caller makes another request in the callback.
  if (!device_type) {
    PostFakeJsError(request_id, "Unknown device");
    return;
  }
  // TODO: Remember the request ID and reply to the transfer when a card
  // insertion/removal is simulated.
}

optional<LibusbJsTransferResult>
TestingSmartCardSimulation::HandleOutputBulkTransferLocked(
    const std::vector<uint8_t>& data_to_send,
    DeviceState& device_state) {
  // Extract the command's sequence number ("bSeq").
  GOOGLE_SMART_CARD_CHECK(data_to_send.size() >= 7);
  uint8_t sequence_number = data_to_send[6];

  switch (data_to_send[0]) {
    case 0x65:
      // PC_to_RDR_GetSlotStatus request to the reader.
      device_state.next_bulk_transfer_reply =
          MakeSlotStatusTransferReply(sequence_number);
      return LibusbJsTransferResult();
    case 0x63:
      // PC_to_RDR_IccPowerOff request to the reader.
      device_state.next_bulk_transfer_reply =
          MakeSlotStatusTransferReply(sequence_number);
      return LibusbJsTransferResult();
    case 0x6B:
      // PC_to_RDR_Escape request to the reader.
      device_state.next_bulk_transfer_reply =
          MakeEscapeTransferReply(sequence_number);
      return LibusbJsTransferResult();
  }
  // Unknown command.
  return {};
}

optional<LibusbJsTransferResult>
TestingSmartCardSimulation::HandleInputBulkTransferLocked(
    int64_t length_to_receive,
    DeviceState& device_state) {
  if (device_state.next_bulk_transfer_reply.empty()) {
    // Unexpected command - we have no reply prepared.
    return {};
  }
  GOOGLE_SMART_CARD_CHECK(device_state.next_bulk_transfer_reply.size() <=
                          length_to_receive);
  LibusbJsTransferResult result;
  result.received_data = std::move(device_state.next_bulk_transfer_reply);
  device_state.next_bulk_transfer_reply.clear();
  return result;
}

}  // namespace google_smart_card
