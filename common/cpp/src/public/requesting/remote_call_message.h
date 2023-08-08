// Copyright 2016 Google Inc. All Rights Reserved.
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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_MESSAGE_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_MESSAGE_H_

#include <string>
#include <vector>

#include "common/cpp/src/public/value.h"

namespace google_smart_card {

// Represents the contents of the `RequestMessageData::payload` field for
// "remote call" requests.
//
// Example usage scenario: Suppose the C++ code wants to make a "promptUser"
// remote call request to the JavaScript side. The simplified code would look
// like this:
//   RemoteCallRequestPayload payload;
//   payload.function_name = "promptUser";
//   payload.arguments.emplace_back("Please enter foo");
//   RequestMessageData message_data;
//   message_data.request_id = 123;
//   message_data.payload = ConvertToValueOrDie(std::move(payload));
//   TypedMessage typed_message;
//   typed_message.type = GetRequestMessageType("promptUser");
//   typed_message.data = ConvertToValueOrDie(std::move(message_data));
//   SendMessageToJs(typed_message);
// The received response would be a typed message that is similar to the one
// produced by this sample code:
//   ResponseMessageData response_message_data;
//   response_message_data.request_id = 123;
//   response_message_data.payload = Value("foo");
//   TypedMessage response_typed_message;
//   response_typed_message.type = GetResponseMessageType("promptUser");
//   response_typed_message.data = ConvertToValueOrDie(
//       std::move(response_message_data));
struct RemoteCallRequestPayload {
  std::string DebugDumpSanitized() const;

  std::string function_name;
  std::vector<Value> arguments;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_MESSAGE_H_
