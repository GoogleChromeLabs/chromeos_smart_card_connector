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
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/requesting/requester_message.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/cpp/src/public/value_debug_dumping.h"

#include "common/cpp/src/public/value_builder.h"

namespace google_smart_card {

namespace {

bool EndsWith(const std::string& string, const std::string& suffix) {
  if (string.length() <= suffix.length() ||
      string.substr(string.length() - suffix.length()) != suffix) {
    return false;
  }
  return true;
}

// Checks the message type against the "...::request" pattern.
bool LooksLikeRequestMessage(const std::string& message_type) {
  return EndsWith(message_type, kRequestMessageTypeSuffix);
}

// Checks the message type against the "...::response" pattern.
bool LooksLikeResponseMessage(const std::string& message_type) {
  return EndsWith(message_type, kResponseMessageTypeSuffix);
}

// `payload_to_reply_with` is passed via shared_ptr in order to workaround
// std::function's lack of support for move-only bounds.
void PostFakeJsReply(TypedMessageRouter* typed_message_router,
                     const std::string& requester_name,
                     std::shared_ptr<Value> payload_to_reply_with,
                     const std::optional<std::string>& error_to_reply_with,
                     std::optional<Value> /*request_payload*/,
                     std::optional<RequestId> request_id) {
  GOOGLE_SMART_CARD_CHECK(request_id);

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

void TestingGlobalContext::Waiter::Reply(Value result_to_reply_with) {
  GOOGLE_SMART_CARD_CHECK(requester_name_);
  // No mutex locks, as it's only allowed to call us after `Wait()` completes.
  GOOGLE_SMART_CARD_CHECK(resolved_);
  GOOGLE_SMART_CARD_CHECK(request_id_);
  // The request result is always wrapped into a single-item array. Do it here,
  // so that the test bodies are easier to read.
  Value array = ArrayValueBuilder().Add(std::move(result_to_reply_with)).Get();
  PostFakeJsReply(typed_message_router_, *requester_name_,
                  std::make_shared<Value>(std::move(array)),
                  /*error_to_reply_with=*/{},
                  /*request_payload=*/{}, *request_id_);
}

const std::optional<Value>& TestingGlobalContext::Waiter::value() const {
  // No mutex locks, as it's only allowed to call us after `Wait()` completes.
  GOOGLE_SMART_CARD_CHECK(resolved_);
  return value_;
}

std::optional<Value> TestingGlobalContext::Waiter::take_value() && {
  // No mutex locks, as it's only allowed to call us after `Wait()` completes.
  GOOGLE_SMART_CARD_CHECK(resolved_);
  return std::move(value_);
}

std::optional<RequestId> TestingGlobalContext::Waiter::request_id() const {
  // No mutex locks, as it's only allowed to call us after `Wait()` completes.
  GOOGLE_SMART_CARD_CHECK(resolved_);
  return request_id_;
}

TestingGlobalContext::Waiter::Waiter(
    TypedMessageRouter* typed_message_router,
    const std::optional<std::string>& requester_name)
    : typed_message_router_(typed_message_router),
      requester_name_(requester_name) {}

void TestingGlobalContext::Waiter::ResolveWithMessageData(Value message_data) {
  std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(!resolved_);
  resolved_ = true;
  value_ = std::move(message_data);
  condition_.notify_one();
}

void TestingGlobalContext::Waiter::ResolveWithRequestPayload(
    RequestId request_id,
    Value request_payload) {
  std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(!resolved_);
  resolved_ = true;
  request_id_ = request_id;
  value_ = std::move(request_payload);
  condition_.notify_one();
}

void TestingGlobalContext::Waiter::ResolveWithResponsePayload(
    RequestId request_id,
    std::optional<Value> response_payload) {
  std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(!resolved_);
  resolved_ = true;
  request_id_ = request_id;
  value_ = std::move(response_payload);
  condition_.notify_one();
}

TestingGlobalContext::TestingGlobalContext(
    TypedMessageRouter* typed_message_router)
    : typed_message_router_(typed_message_router),
      creation_thread_id_(std::this_thread::get_id()) {
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
  return std::this_thread::get_id() == creation_thread_id_ &&
         creation_thread_is_event_loop_;
}

void TestingGlobalContext::ShutDown() {}

void TestingGlobalContext::RegisterMessageHandler(
    const std::string& message_type,
    MessageCallback callback_to_run) {
  Expectation expectation;
  expectation.awaited_message_type = message_type;
  expectation.callback_storage.message_callback = callback_to_run;
  expectation.once = false;
  AddExpectation(std::move(expectation));
}

void TestingGlobalContext::RegisterRequestHandler(
    const std::string& requester_name,
    RequestCallback callback_to_run) {
  Expectation expectation;
  expectation.awaited_message_type = GetRequestMessageType(requester_name);
  expectation.callback_storage.request_callback = callback_to_run;
  expectation.once = false;
  AddExpectation(std::move(expectation));
}

void TestingGlobalContext::RegisterResponseHandler(
    const std::string& requester_name,
    ResponseCallback callback_to_run) {
  Expectation expectation;
  expectation.awaited_message_type = GetResponseMessageType(requester_name);
  expectation.callback_storage.response_callback = callback_to_run;
  expectation.once = false;
  AddExpectation(std::move(expectation));
}

void TestingGlobalContext::RegisterRequestRerouter(
    const std::string& original_requester_name,
    const std::string& new_requester_name) {
  // Reroute requests to the new name.
  RegisterRequestHandler(
      original_requester_name,
      std::bind(&TestingGlobalContext::HandleReroutedRequest, this,
                /*new_requester_name=*/new_requester_name,
                /*request_id=*/std::placeholders::_1,
                /*request_payload=*/std::placeholders::_2));
  // Reroute responses back to the original name.
  RegisterResponseHandler(
      new_requester_name,
      std::bind(&TestingGlobalContext::HandleReroutedResponse, this,
                /*new_requester_name=*/original_requester_name,
                /*request_id=*/std::placeholders::_1,
                /*response_payload=*/std::placeholders::_2,
                /*response_error_message=*/std::placeholders::_3));
}

std::unique_ptr<TestingGlobalContext::Waiter>
TestingGlobalContext::CreateMessageWaiter(
    const std::string& awaited_message_type) {
  // `std::make_unique` wouldn't work due to the constructor being private.
  std::unique_ptr<Waiter> waiter(
      new Waiter(typed_message_router_, /*requester_name=*/{}));
  Expectation expectation;
  expectation.awaited_message_type = awaited_message_type;
  expectation.callback_storage.message_callback =
      std::bind(&Waiter::ResolveWithMessageData, waiter.get(),
                /*message_data=*/std::placeholders::_1);
  AddExpectation(std::move(expectation));
  return waiter;
}

std::unique_ptr<TestingGlobalContext::Waiter>
TestingGlobalContext::CreateRequestWaiter(const std::string& requester_name,
                                          const std::string& function_name,
                                          Value arguments) {
  // `std::make_unique` wouldn't work due to the constructor being private.
  std::unique_ptr<Waiter> waiter(
      new Waiter(typed_message_router_, requester_name));
  auto callback_to_run =
      std::bind(&Waiter::ResolveWithRequestPayload, waiter.get(),
                /*request_id=*/std::placeholders::_1,
                /*request_payload=*/std::placeholders::_2);
  AddExpectation(MakeRequestExpectation(requester_name, function_name,
                                        std::move(arguments), callback_to_run));
  return waiter;
}

std::unique_ptr<TestingGlobalContext::Waiter>
TestingGlobalContext::CreateResponseWaiter(const std::string& requester_name,
                                           RequestId request_id) {
  // `std::make_unique` wouldn't work due to the constructor being private.
  std::unique_ptr<Waiter> waiter(
      new Waiter(typed_message_router_, requester_name));
  Expectation expectation;
  expectation.awaited_message_type = GetResponseMessageType(requester_name);
  expectation.awaited_request_id = request_id;
  expectation.callback_storage.response_callback =
      std::bind(&Waiter::ResolveWithResponsePayload, waiter.get(),
                /*request_id=*/std::placeholders::_1,
                /*request_payload=*/std::placeholders::_2);
  AddExpectation(std::move(expectation));
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
      payload_shared_ptr, /*error_to_reply_with=*/std::optional<std::string>(),
      /*request_payload=*/std::placeholders::_2,
      /*request_id=*/std::placeholders::_1);

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
      /*request_payload=*/std::placeholders::_2,
      /*request_id=*/std::placeholders::_1);

  AddExpectation(MakeRequestExpectation(requester_name, function_name,
                                        std::move(arguments), callback_to_run));
}

TestingGlobalContext::Expectation TestingGlobalContext::MakeRequestExpectation(
    const std::string& requester_name,
    const std::string& function_name,
    Value arguments,
    RequestCallback callback_to_run) {
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
  expectation.callback_storage.request_callback = callback_to_run;
  return expectation;
}

void TestingGlobalContext::AddExpectation(Expectation expectation) {
  const std::unique_lock<std::mutex> lock(mutex_);

  expectations_.push_back(std::move(expectation));
}

std::optional<TestingGlobalContext::CallbackStorage>
TestingGlobalContext::FindMatchingExpectation(
    const std::string& message_type,
    std::optional<RequestId> request_id,
    const Value* request_payload) {
  std::unique_lock<std::mutex> lock(mutex_);

  for (auto iter = expectations_.begin(); iter != expectations_.end(); ++iter) {
    const Expectation& expectation = *iter;
    if (message_type != expectation.awaited_message_type) {
      // Skip - different type.
      continue;
    }
    if (expectation.awaited_request_id && !request_id) {
      // Skip - expected a message with a request ID, but none got.
      continue;
    }
    if (expectation.awaited_request_id &&
        *expectation.awaited_request_id != *request_id) {
      // Skip - different request ID.
      continue;
    }
    if (expectation.awaited_request_payload && !request_payload) {
      // Skip - expected a request message with a payload, but none got.
      continue;
    }
    if (expectation.awaited_request_payload &&
        !expectation.awaited_request_payload->StrictlyEquals(
            *request_payload)) {
      // Skip - different payload.
      continue;
    }
    // A match found. Remove the expectation if it's a one-time.
    CallbackStorage result = expectation.callback_storage;
    if (expectation.once)
      expectations_.erase(iter);
    return result;
  }
  // No match found.
  return {};
}

bool TestingGlobalContext::HandleMessageToJs(Value message) {
  TypedMessage typed_message =
      ConvertFromValueOrDie<TypedMessage>(std::move(message));

  std::optional<RequestId> request_id;
  // Only one of these variables will be filled below - depending on the message
  // type.
  std::optional<Value> request_payload, response_payload, message_data;
  std::optional<std::string> response_error_message;

  if (LooksLikeRequestMessage(typed_message.type)) {
    // It's a request message; parse its ID and payload for finding the
    // expectation and passing them to the callback.
    RequestMessageData request_data = ConvertFromValueOrDie<RequestMessageData>(
        std::move(typed_message.data));
    request_id = request_data.request_id;
    request_payload = std::move(request_data.payload);
  } else if (LooksLikeResponseMessage(typed_message.type)) {
    // It's a response message; parse ID for finding the expectation and the
    // payload (which can be null if the response contains a failure) for
    // passing to the callback.
    ResponseMessageData response_data =
        ConvertFromValueOrDie<ResponseMessageData>(
            std::move(typed_message.data));
    request_id = response_data.request_id;
    response_payload = std::move(response_data.payload);
    response_error_message = std::move(response_data.error_message);
  } else {
    // It's a regular message; find the expectation using just the type.
    message_data = std::move(typed_message.data);
  }

  // Find the callback for the message type, request ID and, if it's a request
  // message, the request payload.
  std::optional<CallbackStorage> callback_storage =
      FindMatchingExpectation(typed_message.type, request_id,
                              request_payload ? &*request_payload : nullptr);
  if (!callback_storage)
    return false;

  // Run the provided callback (it's stored in one of the fields of the
  // union-like struct).
  if (callback_storage->message_callback) {
    GOOGLE_SMART_CARD_CHECK(message_data);
    callback_storage->message_callback(std::move(*message_data));
  } else if (callback_storage->request_callback) {
    GOOGLE_SMART_CARD_CHECK(request_id);
    GOOGLE_SMART_CARD_CHECK(request_payload);
    callback_storage->request_callback(*request_id,
                                       std::move(*request_payload));
  } else if (callback_storage->response_callback) {
    GOOGLE_SMART_CARD_CHECK(request_id);
    callback_storage->response_callback(*request_id,
                                        std::move(response_payload),
                                        std::move(response_error_message));
  } else {
    GOOGLE_SMART_CARD_NOTREACHED;
  }

  return true;
}

void TestingGlobalContext::HandleReroutedRequest(
    const std::string& new_requester_name,
    RequestId request_id,
    Value request_payload) {
  RequestMessageData new_data;
  new_data.request_id = request_id;
  new_data.payload = std::move(request_payload);
  TypedMessage new_message;
  new_message.type = GetRequestMessageType(new_requester_name);
  new_message.data = ConvertToValueOrDie(std::move(new_data));
  Value new_request = ConvertToValueOrDie(std::move(new_message));

  std::string error_message;
  if (!typed_message_router_->OnMessageReceived(std::move(new_request),
                                                &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Dispatching rerouted JS request failed: "
                                << error_message;
  }
}

void TestingGlobalContext::HandleReroutedResponse(
    const std::string& new_requester_name,
    RequestId request_id,
    std::optional<Value> response_payload,
    std::optional<std::string> response_error_message) {
  ResponseMessageData new_data;
  new_data.request_id = request_id;
  new_data.payload = std::move(response_payload);
  new_data.error_message = std::move(response_error_message);
  TypedMessage new_message;
  new_message.type = GetResponseMessageType(new_requester_name);
  new_message.data = ConvertToValueOrDie(std::move(new_data));
  Value new_response = ConvertToValueOrDie(std::move(new_message));

  std::string error_message;
  if (!typed_message_router_->OnMessageReceived(std::move(new_response),
                                                &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Dispatching rerouted JS response failed: "
                                << error_message;
  }
}

}  // namespace google_smart_card
