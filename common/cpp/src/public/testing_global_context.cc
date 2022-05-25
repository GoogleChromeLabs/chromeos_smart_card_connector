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

#include "common/cpp/src/public/testing_global_context.h"

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_debug_dumping.h>

namespace google_smart_card {

TestingGlobalContext::TestingGlobalContext(
    TypedMessageRouter* typed_message_router)
    : typed_message_router_(typed_message_router) {
  GOOGLE_SMART_CARD_CHECK(typed_message_router_);
}

TestingGlobalContext::~TestingGlobalContext() = default;

void TestingGlobalContext::PostMessageToJs(Value message) {
  const std::string debug_dump = DebugDumpValueFull(message);
  if (!HandleMessageToJs(std::move(message))) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected message posted to JS: "
                                << debug_dump;
  }
}

bool TestingGlobalContext::IsMainEventLoopThread() const {
  return creation_thread_is_event_loop_;
}

void TestingGlobalContext::ShutDown() {}

void TestingGlobalContext::WillReplyToRequestWith(
    const std::string& requester_name,
    const std::string& function_name,
    Value arguments,
    Value result_to_reply_with) {
  // The request result is always wrapped into a single-item array. Do it
  // here, so that the test bodies are easier to read.
  Value::ArrayStorage array;
  array.push_back(MakeUnique<Value>(std::move(result_to_reply_with)));
  AddExpectation(MakeRequestExpectation(
      requester_name, function_name, std::move(arguments),
      /*payload_to_reply_with=*/Value(std::move(array)),
      /*error_to_reply_with=*/{}));
}

void TestingGlobalContext::WillReplyToRequestWithError(
    const std::string& requester_name,
    const std::string& function_name,
    Value arguments,
    const std::string& error_to_reply_with) {
  AddExpectation(MakeRequestExpectation(
      requester_name, function_name, std::move(arguments),
      /*payload_to_reply_with=*/{}, error_to_reply_with));
}

// static
TestingGlobalContext::Expectation TestingGlobalContext::MakeRequestExpectation(
    const std::string& requester_name,
    const std::string& function_name,
    Value arguments,
    optional<Value> payload_to_reply_with,
    const optional<std::string>& error_to_reply_with) {
  GOOGLE_SMART_CARD_CHECK(arguments.is_array());

  RemoteCallRequestPayload request_payload;
  request_payload.function_name = function_name;
  // Convert `Value` to `vector<Value>`. Ideally the conversion wouldn't be
  // needed, but in tests it's more convenient pass a single `Value` (e.g.,
  // constructed via `ArrayValueBuilder`), meanwhile in the
  // `RemoteCallRequestPayload` struct definition we want to express that only
  // array values are allowed.
  request_payload.arguments =
      ConvertFromValueOrDie<std::vector<Value>>(std::move(arguments));

  Expectation expectation;
  expectation.requester_name = requester_name;
  expectation.awaited_request_payload =
      ConvertToValueOrDie(std::move(request_payload));
  expectation.payload_to_reply_with = std::move(payload_to_reply_with);
  expectation.error_to_reply_with = error_to_reply_with;
  return expectation;
}

void TestingGlobalContext::AddExpectation(Expectation expectation) {
  std::unique_lock<std::mutex> lock(mutex_);

  expectations_.push_back(std::move(expectation));
}

optional<TestingGlobalContext::Expectation>
TestingGlobalContext::PopMatchingExpectation(const std::string& message_type,
                                             const Value& request_payload) {
  std::unique_lock<std::mutex> lock(mutex_);

  for (auto iter = expectations_.begin(); iter != expectations_.end(); ++iter) {
    if (message_type == GetRequestMessageType(iter->requester_name) &&
        request_payload.StrictlyEquals(iter->awaited_request_payload)) {
      // A match found. The expectation is a one of, so remove it.
      Expectation match = std::move(*iter);
      expectations_.erase(iter);
      return std::move(match);
    }
  }
  // No match found.
  return {};
}

bool TestingGlobalContext::HandleMessageToJs(Value message) {
  TypedMessage typed_message =
      ConvertFromValueOrDie<TypedMessage>(std::move(message));
  RequestMessageData request_data =
      ConvertFromValueOrDie<RequestMessageData>(std::move(typed_message.data));

  optional<Expectation> expectation =
      PopMatchingExpectation(typed_message.type, request_data.payload);
  if (!expectation)
    return false;

  PostFakeJsReply(request_data.request_id, expectation->requester_name,
                  std::move(expectation->payload_to_reply_with),
                  expectation->error_to_reply_with);
  return true;
}

void TestingGlobalContext::PostFakeJsReply(
    RequestId request_id,
    const std::string& requester_name,
    optional<Value> payload_to_reply_with,
    const optional<std::string>& error_to_reply_with) {
  ResponseMessageData response_data;
  response_data.request_id = request_id;
  response_data.payload = std::move(payload_to_reply_with);
  response_data.error_message = std::move(error_to_reply_with);

  TypedMessage response;
  response.type = GetResponseMessageType(requester_name);
  response.data = ConvertToValueOrDie(std::move(response_data));

  Value reply = ConvertToValueOrDie(std::move(response));

  std::string error_message;
  if (!typed_message_router_->OnMessageReceived(std::move(reply),
                                                &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Dispatching fake JS reply failed: "
                                << error_message;
  }
}

}  // namespace google_smart_card
