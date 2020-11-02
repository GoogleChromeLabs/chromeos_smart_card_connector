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

#include <google_smart_card_common/requesting/requester_message.h>

#include <string>
#include <utility>

#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

namespace {

constexpr char kRequestMessageTypeSuffix[] = "::request";
constexpr char kResponseMessageTypeSuffix[] = "::response";
constexpr char kCanceledErrorMessage[] = "The request was canceled";

}  // namespace

std::string GetRequestMessageType(const std::string& name) {
  return name + kRequestMessageTypeSuffix;
}

std::string GetResponseMessageType(const std::string& name) {
  return name + kResponseMessageTypeSuffix;
}

// Register the structs for conversions to/from `Value`.

template <>
StructValueDescriptor<RequestMessageData>::Description
StructValueDescriptor<RequestMessageData>::GetDescription() {
  // Note: Strings passed to WithField() below must match the keys in
  // //common/js/src/requesting/requester-message.js.
  return Describe("RequestMessageData")
      .WithField(&RequestMessageData::request_id, "request_id")
      .WithField(&RequestMessageData::payload, "payload");
}

template <>
StructValueDescriptor<ResponseMessageData>::Description
StructValueDescriptor<ResponseMessageData>::GetDescription() {
  // Note: Strings passed to WithField() below must match the keys in
  // //common/js/src/requesting/requester-message.js.
  return Describe("ResponseMessageData")
      .WithField(&ResponseMessageData::request_id, "request_id")
      .WithField(&ResponseMessageData::payload, "payload")
      .WithField(&ResponseMessageData::error_message, "error");
}

// static
ResponseMessageData ResponseMessageData::CreateFromRequestResult(
    RequestId request_id, GenericRequestResult request_result) {
  ResponseMessageData message_data;
  message_data.request_id = request_id;
  switch (request_result.status()) {
    case RequestResultStatus::kSucceeded:
      message_data.payload = std::move(request_result).TakePayload();
      break;
    case RequestResultStatus::kFailed:
      message_data.error_message = request_result.error_message();
      break;
    case RequestResultStatus::kCanceled:
      message_data.error_message = std::string(kCanceledErrorMessage);
      break;
  }
  return message_data;
}

bool ResponseMessageData::ExtractRequestResult(
    GenericRequestResult* request_result) {
  if (payload && !error_message) {
    *request_result =
        GenericRequestResult::CreateSuccessful(std::move(*payload));
    return true;
  }
  if (error_message && !payload) {
    *request_result = GenericRequestResult::CreateFailed(*error_message);
    return true;
  }
  // Only one of (`error_message`, `payload`) should be provided.
  return false;
}

}  // namespace google_smart_card
