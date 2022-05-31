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

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Implements fake smart card reader USB devices.
class TestingSmartCardSimulation final {
 public:
  static const char* kRequesterName;

  explicit TestingSmartCardSimulation(TypedMessageRouter* typed_message_router);
  TestingSmartCardSimulation(const TestingSmartCardSimulation&) = delete;
  TestingSmartCardSimulation& operator=(const TestingSmartCardSimulation&) =
      delete;
  ~TestingSmartCardSimulation();

  // Subscribe this to the C++-to-JS message channel.
  void OnRequestToJs(Value request_payload, optional<RequestId> request_id);

 private:
  void PostFakeJsReply(RequestId request_id, Value payload_to_reply_with);
  void OnListDevicesCalled(RequestId request_id);

  TypedMessageRouter* const typed_message_router_;
};

}  // namespace google_smart_card

#endif  // SMART_CARD_CONNECTOR_APP_TESTING_SMART_CARD_SIMULATION_H_
