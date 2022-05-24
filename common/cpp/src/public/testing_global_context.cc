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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
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

#include "common/cpp/src/public/value_builder.h"

namespace google_smart_card {

namespace {

// Parses out the requester name from "<requester>::request", or returns a null
// optional if the format doesn't match.
optional<std::string> GetRequesterName(const std::string& message_type) {
  const std::string suffix = kRequestMessageTypeSuffix;
  if (message_type.length() <= suffix.length() ||
      message_type.substr(message_type.length() - suffix.length()) != suffix) {
    // Not a requester message.
    return {};
  }
  return message_type.substr(0, message_type.length() - suffix.length());
}

// `payload_to_reply_with` is passed via shared_ptr in order to workaround
// std::function's lack of support for move-only bounds.
void PostFakeJsReply(TypedMessageRouter* typed_message_router,
                     const std::string& requester_name,
                     std::shared_ptr<Value> payload_to_reply_with,
                     const optional<std::string>& error_to_reply_with,
                     Value /*request_payload*/,
                     optional<RequestId> request_id) {
  ResponseMessageData response_data;
  response_data.request_id = *request_id;
  if (payload_to_reply_with)
    response_data.payload = std::move(*payload_to_reply_with);
  response_data.error_message = std::move(error_to_reply_with);

  TypedMessage response;
  response.type = GetResponseMessageType(requester_name);
  response.data = ConvertToValueOrDie(std::move(response_data));

  Value reply = ConvertToValueOrDie(std::move(response));

  std::string error_message;
  if (!typed_message_router->OnMessageReceived(std::move(reply),
                                               &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Dispatching fake JS reply failed: "
                                << error_message;
  }
}

}  // namespace

TestingGlobalContext::Waiter::~Waiter() = default;

void TestingGlobalContext::Waiter::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  // Wait until `Resolve()` gets called.
  condition_.wait(lock, [&] { return resolved_; });
}

const Value& TestingGlobalContext::Waiter::value() const {
  // No mutex locks, as it's only allowed to call us after `Wait()` completes.
  GOOGLE_SMART_CARD_CHECK(resolved_);
  return value_;
}

Value TestingGlobalContext::Waiter::take_value() && {
  // No mutex locks, as it's only allowed to call us after `Wait()` completes.
  GOOGLE_SMART_CARD_CHECK(resolved_);
  return std::move(value_);
}

optional<RequestId> TestingGlobalContext::Waiter::request_id() const {
  // No mutex locks, as it's only allowed to call us after `Wait()` completes.
  GOOGLE_SMART_CARD_CHECK(resolved_);
  return request_id_;
}

TestingGlobalContext::Waiter::Waiter() = default;

void TestingGlobalContext::Waiter::Resolve(Value value,
                                           optional<RequestId> request_id) {
  std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(!resolved_);
  resolved_ = true;
  value_ = std::move(value);
  request_id_ = request_id;
  condition_.notify_one();
}

TestingGlobalContext::TestingGlobalContext(
    TypedMessageRouter* typed_message_router)
    : typed_message_router_(typed_message_router),
      main_thread_id_(std::this_thread::get_id()) {
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
  return std::this_thread::get_id() == main_thread_id_ &&
         creation_thread_is_event_loop_;
}

void TestingGlobalContext::ShutDown() {}

std::unique_ptr<TestingGlobalContext::Waiter>
TestingGlobalContext::CreateMessageWaiter(
    const std::string& awaited_message_type) {
  // `MakeUnique` wouldn't work due to the constructor being private.
  std::unique_ptr<Waiter> waiter(new Waiter);
  Expectation expectation;
  expectation.awaited_message_type = awaited_message_type;
  expectation.callback_to_run =
      std::bind(&Waiter::Resolve, waiter.get(), /*value=*/std::placeholders::_1,
                /*request_id=*/std::placeholders::_2);
  AddExpectation(std::move(expectation));
  return waiter;
}

std::unique_ptr<TestingGlobalContext::Waiter>
TestingGlobalContext::CreateRequestWaiter(const std::string& requester_name,
                                          const std::string& function_name,
                                          Value arguments) {
  // `MakeUnique` wouldn't work due to the constructor being private.
  std::unique_ptr<Waiter> waiter(new Waiter);
  auto callback_to_run =
      std::bind(&Waiter::Resolve, waiter.get(), /*value=*/std::placeholders::_1,
                /*request_id=*/std::placeholders::_2);
  AddExpectation(MakeRequestExpectation(requester_name, function_name,
                                        std::move(arguments), callback_to_run));
  return waiter;
}

void TestingGlobalContext::WillReplyToRequestWith(
    const std::string& requester_name,
    const std::string& function_name,
    Value arguments,
    Value result_to_reply_with) {
  // The request result is always wrapped into a single-item array. Do it
  // here, so that the test bodies are easier to read.
  Value array = ArrayValueBuilder().Add(std::move(result_to_reply_with)).Get();
  // Work around std::function's inability to bind a move-only argument.
  auto payload_shared_ptr = std::make_shared<Value>(std::move(array));
  auto callback_to_run = std::bind(
      &PostFakeJsReply, typed_message_router_, requester_name,
      payload_shared_ptr, /*error_to_reply_with=*/optional<std::string>(),
      /*request_payload=*/std::placeholders::_1,
      /*request_id=*/std::placeholders::_2);

  AddExpectation(MakeRequestExpectation(requester_name, function_name,
                                        std::move(arguments), callback_to_run));
}

void TestingGlobalContext::WillReplyToRequestWithError(
    const std::string& requester_name,
    const std::string& function_name,
    Value arguments,
    const std::string& error_to_reply_with) {
  auto callback_to_run = std::bind(
      &PostFakeJsReply, typed_message_router_, requester_name,
      /*payload_to_reply_with=*/std::shared_ptr<Value>(), error_to_reply_with,
      /*request_payload=*/std::placeholders::_1,
      /*request_id=*/std::placeholders::_2);

  AddExpectation(MakeRequestExpectation(requester_name, function_name,
                                        std::move(arguments), callback_to_run));
}

TestingGlobalContext::Expectation TestingGlobalContext::MakeRequestExpectation(
    const std::string& requester_name,
    const std::string& function_name,
    Value arguments,
    std::function<void(Value, optional<RequestId>)> callback_to_run) {
  GOOGLE_SMART_CARD_CHECK(arguments.is_array());

  RemoteCallRequestPayload request_payload;
  request_payload.function_name = function_name;
  // Convert an array `Value` to `vector<Value>`. Ideally the conversion
  // wouldn't be needed, but in tests it's more convenient pass a single `Value`
  // (e.g., constructed via `ArrayValueBuilder`), meanwhile in the
  // `RemoteCallRequestPayload` struct definition we want to express that only
  // array values are allowed.
  request_payload.arguments =
      ConvertFromValueOrDie<std::vector<Value>>(std::move(arguments));

  Expectation expectation;
  expectation.awaited_message_type = GetRequestMessageType(requester_name);
  expectation.awaited_request_payload =
      ConvertToValueOrDie(std::move(request_payload));
  expectation.callback_to_run = callback_to_run;
  return expectation;
}

void TestingGlobalContext::AddExpectation(Expectation expectation) {
  const std::unique_lock<std::mutex> lock(mutex_);

  expectations_.push_back(std::move(expectation));
}

optional<TestingGlobalContext::Expectation>
TestingGlobalContext::PopMatchingExpectation(const std::string& message_type,
                                             const Value* request_payload) {
  std::unique_lock<std::mutex> lock(mutex_);

  for (auto iter = expectations_.begin(); iter != expectations_.end(); ++iter) {
    const Expectation& expectation = *iter;
    if (message_type != expectation.awaited_message_type) {
      // Skip - different type.
      continue;
    }
    if (expectation.awaited_request_payload && !request_payload) {
      // Skip - expected a message with a payload, but none got.
      continue;
    }
    if (expectation.awaited_request_payload &&
        !expectation.awaited_request_payload->StrictlyEquals(
            *request_payload)) {
      // Skip - different payload.
      continue;
    }
    // A match found. The expectation is a one of, so remove it.
    Expectation result = std::move(*iter);
    expectations_.erase(iter);
    return std::move(result);
  }
  // No match found.
  return {};
}

bool TestingGlobalContext::HandleMessageToJs(Value message) {
  TypedMessage typed_message =
      ConvertFromValueOrDie<TypedMessage>(std::move(message));

  optional<std::string> requester_name = GetRequesterName(typed_message.type);
  if (requester_name) {
    // It's a request message - parse its payload and use it to find the
    // expectation.
    RequestMessageData request_data = ConvertFromValueOrDie<RequestMessageData>(
        std::move(typed_message.data));
    optional<Expectation> expectation =
        PopMatchingExpectation(typed_message.type, &request_data.payload);
    if (!expectation)
      return false;
    expectation->callback_to_run(std::move(request_data.payload),
                                 request_data.request_id);
    return true;
  }

  // It's a regular message - find the expectation using just the type.
  optional<Expectation> expectation =
      PopMatchingExpectation(typed_message.type, /*request_payload=*/nullptr);
  if (!expectation)
    return false;
  expectation->callback_to_run(std::move(typed_message.data),
                               /*request_id=*/optional<RequestId>());
  return true;
}

}  // namespace google_smart_card
