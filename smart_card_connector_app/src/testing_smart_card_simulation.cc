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

#include <string>
#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_debug_dumping.h>

#include "common/cpp/src/public/value_builder.h"

namespace google_smart_card {

const char* TestingSmartCardSimulation::kRequesterName = "libusb";

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
  GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected request: " << payload_debug_dump;
}

void TestingSmartCardSimulation::PostFakeJsReply(RequestId request_id,
                                                 Value payload_to_reply_with) {
  ResponseMessageData response_data;
  response_data.request_id = request_id;
  response_data.payload = std::move(payload_to_reply_with);

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

void TestingSmartCardSimulation::OnListDevicesCalled(RequestId request_id) {
  // Pretend to have an empty list of devices.
  PostFakeJsReply(request_id,
                  ArrayValueBuilder().Add(Value(Value::Type::kArray)).Get());
}

}  // namespace google_smart_card
