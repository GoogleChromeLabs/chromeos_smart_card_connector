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
#include <queue>
#include <set>
#include <string>
#include <vector>

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>

#include "third_party/libusb/webport/src/libusb_js_proxy_data_model.h"

namespace google_smart_card {

// Implements fake smart card reader USB devices.
//
// The intention here is to have an emulator that mimicks key aspects of
// real-world devices, to allow for unit testing of our low-level components
// like the PC/SC daemon and the CCID driver. This class is NOT meant to provide
// feature-complete fake devices (e.g., it doesn't even do any real
// cryptography), it can't be used as a "virtual smart card" for performing real
// authentication, we don't perform all checks that a real device would do, and
// we don't cover exotic aspects of the specs.
//
// The implementation is based on the protocols standardized in "Specification
// for Integrated Circuit(s) Cards Interface Devices", ISO/IEC 7816-3, ISO/IEC
// 7816-4 and NIST 800-73-4. We focus primarily on flows and commands seen in
// USB logs sniffed from real devices.
class TestingSmartCardSimulation final {
 public:
  // Fake device to simulate.
  enum class DeviceType { kGemaltoPcTwinReader, kDellSmartCardReaderKeyboard };
  enum class CardType { kCosmoId70 };
  enum class CardProfile { kCharismathicsPiv };

  // Represents whether an ICC (a smart card) is inserted into the reader and is
  // powered. Corresponds to "bmICCStatus" from CCID specs.
  enum class CcidIccStatus : uint8_t {
    kPresentActive = 0,
    kPresentInactive = 1,
    kNotPresent = 2,
  };

  // Parameters of the simulated device.
  struct Device {
    // Unique device identifier to be used in the fake JS replies.
    int64_t id = -1;
    DeviceType type;
    // A null value denotes "no card inserted".
    optional<CardType> card_type;
    // A null value denotes "the card is uninitialized".
    optional<CardProfile> card_profile;
  };

  static const char* kRequesterName;

  explicit TestingSmartCardSimulation(TypedMessageRouter* typed_message_router);
  TestingSmartCardSimulation(const TestingSmartCardSimulation&) = delete;
  TestingSmartCardSimulation& operator=(const TestingSmartCardSimulation&) =
      delete;
  ~TestingSmartCardSimulation();

  // Subscribe this to the C++-to-JS message channel.
  void OnRequestToJs(RequestId request_id, Value request_payload);

  void SetDevices(const std::vector<Device>& devices);

  // Returns an ATR (answer-to-reset) for the given simulated card.
  static std::vector<uint8_t> GetCardAtr(CardType card_type);
  // Returns an identifier of the card applet. The format follows ISO/IEC
  // 7816-4.
  static std::vector<uint8_t> GetCardProfileApplicationIdentifier(
      CardProfile card_profile);

 private:
  // The simulation state of a device.
  struct DeviceState {
    Device device;
    optional<int64_t> opened_device_handle;
    std::set<int64_t> claimed_interfaces;
    std::vector<uint8_t> next_bulk_transfer_reply;
    std::queue<RequestId> pending_interrupt_transfers;
    CcidIccStatus icc_status = CcidIccStatus::kNotPresent;
  };

  // Helper class that provides thread-safe operations with reader states.
  class ThreadSafeHandler final {
   public:
    using DeviceState = TestingSmartCardSimulation::DeviceState;

    explicit ThreadSafeHandler(TypedMessageRouter* typed_message_router);
    ThreadSafeHandler(const ThreadSafeHandler&) = delete;
    ThreadSafeHandler& operator=(const ThreadSafeHandler&) = delete;
    ~ThreadSafeHandler();

    void SetDevices(const std::vector<Device>& devices);

    // Fake implementation of the JS counterpart of the Libusb operations:
    GenericRequestResult ListDevices();
    GenericRequestResult GetConfigurations(int64_t device_id);
    GenericRequestResult OpenDeviceHandle(int64_t device_id);
    GenericRequestResult CloseDeviceHandle(int64_t device_id,
                                           int64_t device_handle);
    GenericRequestResult ClaimInterface(int64_t device_id,
                                        int64_t device_handle,
                                        int64_t interface_number);
    GenericRequestResult ReleaseInterface(int64_t device_id,
                                          int64_t device_handle,
                                          int64_t interface_number);
    GenericRequestResult ControlTransfer(
        int64_t device_id,
        int64_t device_handle,
        LibusbJsControlTransferParameters params);
    GenericRequestResult BulkTransfer(int64_t device_id,
                                      int64_t device_handle,
                                      LibusbJsGenericTransferParameters params);
    optional<GenericRequestResult> InterruptTransfer(
        RequestId request_id,
        int64_t device_id,
        int64_t device_handle,
        LibusbJsGenericTransferParameters params);

   private:
    DeviceState* FindDeviceStateById(int64_t device_id);
    DeviceState* FindDeviceStateByIdAndHandle(int64_t device_id,
                                              int64_t device_handle);
    void UpdateDeviceState(const Device& device, DeviceState& device_state);
    void NotifySlotChange(DeviceState& device_state);

    GenericRequestResult HandleOutputBulkTransfer(
        const std::vector<uint8_t>& data_to_send,
        DeviceState& device_state);
    GenericRequestResult HandleInputBulkTransfer(int64_t length_to_receive,
                                                 DeviceState& device_state);

    TypedMessageRouter* const typed_message_router_;
    std::mutex mutex_;
    std::vector<DeviceState> device_states_;
    int64_t next_free_device_handle_ = 1;
  };

  TypedMessageRouter* const typed_message_router_;
  ThreadSafeHandler handler_;
};

}  // namespace google_smart_card

#endif  // SMART_CARD_CONNECTOR_APP_TESTING_SMART_CARD_SIMULATION_H_
