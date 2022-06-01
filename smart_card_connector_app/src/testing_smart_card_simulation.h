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

#ifndef SMART_CARD_CONNECTOR_APP_TESTING_SMART_CARD_SIMULATION_H_
#define SMART_CARD_CONNECTOR_APP_TESTING_SMART_CARD_SIMULATION_H_

#include <stdint.h>

#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>

#include "third_party/libusb/webport/src/libusb_js_proxy_data_model.h"

namespace google_smart_card {

// Implements fake smart card reader USB devices.
class TestingSmartCardSimulation final {
 public:
  // Fake device to simulate.
  enum class DeviceType { kGemaltoPcTwinReader };

  // Parameters of the simulated device.
  struct Device {
    // Unique device identifier to be used in the fake JS replies.
    int64_t id = -1;
    DeviceType type;
  };

  static const char* kRequesterName;

  explicit TestingSmartCardSimulation(TypedMessageRouter* typed_message_router);
  TestingSmartCardSimulation(const TestingSmartCardSimulation&) = delete;
  TestingSmartCardSimulation& operator=(const TestingSmartCardSimulation&) =
      delete;
  ~TestingSmartCardSimulation();

  // Subscribe this to the C++-to-JS message channel.
  void OnRequestToJs(Value request_payload, optional<RequestId> request_id);

  void SetDevices(const std::vector<Device>& devices);

 private:
  // The simulation state of a device.
  struct DeviceState {
    Device device;
    optional<int64_t> opened_device_handle;
    std::set<int64_t> claimed_interfaces;
    std::vector<uint8_t> next_bulk_transfer_reply;
  };

  void PostFakeJsResult(RequestId request_id, Value result);
  void PostFakeJsError(RequestId request_id, const std::string& error);
  void PostResponseMessage(ResponseMessageData response_data);

  DeviceState* FindDeviceStateByIdLocked(int64_t device_id);
  DeviceState* FindDeviceStateByIdAndHandleLocked(int64_t device_id,
                                                  int64_t device_handle);

  void OnListDevicesCalled(RequestId request_id);
  void OnGetConfigurationsCalled(int64_t device_id, RequestId request_id);
  void OnOpenDeviceHandleCalled(int64_t device_id, RequestId request_id);
  void OnCloseDeviceHandleCalled(int64_t device_id,
                                 int64_t device_handle,
                                 RequestId request_id);
  void OnClaimInterfaceCalled(int64_t device_id,
                              int64_t device_handle,
                              int64_t interface_number,
                              RequestId request_id);
  void OnReleaseInterfaceCalled(int64_t device_id,
                                int64_t device_handle,
                                int64_t interface_number,
                                RequestId request_id);
  void OnControlTransferCalled(int64_t device_id,
                               int64_t device_handle,
                               LibusbJsControlTransferParameters params,
                               RequestId request_id);
  void OnBulkTransferCalled(int64_t device_id,
                            int64_t device_handle,
                            LibusbJsGenericTransferParameters params,
                            RequestId request_id);
  void OnInterruptTransferCalled(int64_t device_id,
                                 int64_t device_handle,
                                 LibusbJsGenericTransferParameters params,
                                 RequestId request_id);

  optional<LibusbJsTransferResult> HandleOutputBulkTransferLocked(
      const std::vector<uint8_t>& data_to_send,
      DeviceState& device_state);
  optional<LibusbJsTransferResult> HandleInputBulkTransferLocked(
      int64_t length_to_receive,
      DeviceState& device_state);

  TypedMessageRouter* const typed_message_router_;
  std::mutex mutex_;
  std::vector<DeviceState> device_states_;
  int64_t next_free_device_handle_ = 1;
};

}  // namespace google_smart_card

#endif  // SMART_CARD_CONNECTOR_APP_TESTING_SMART_CARD_SIMULATION_H_
