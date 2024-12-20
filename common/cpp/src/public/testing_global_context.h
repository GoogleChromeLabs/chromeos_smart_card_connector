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

#ifndef GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_TESTING_GLOBAL_CONTEXT_H_
#define GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_TESTING_GLOBAL_CONTEXT_H_

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/requesting/request_id.h"
#include "common/cpp/src/public/requesting/requester_message.h"
#include "common/cpp/src/public/value.h"

namespace google_smart_card {

// Test implementation of `GlobalContext` that allows to set up expectations on
// the messages sent to the JavaScript side and to simulate responses from it.
class TestingGlobalContext final : public GlobalContext {
 public:
  // Callbacks to be run when an expected message is sent to JS.
  using MessageCallback = std::function<void(Value message_data)>;
  using RequestCallback =
      std::function<void(RequestId request_id, Value request_payload)>;
  using ResponseCallback =
      std::function<void(RequestId request_id,
                         std::optional<Value> response_payload,
                         std::optional<std::string> response_error_message)>;

  // Helper returned by `Create...Waiter()` methods. Allows to wait until the
  // specified C++-to-JS message is sent.
  class Waiter final {
   public:
    Waiter(const Waiter&) = delete;
    Waiter& operator=(const Waiter&) = delete;
    ~Waiter();

    void Wait();
    void Reply(Value result_to_reply_with);

    const std::optional<Value>& value() const;
    std::optional<Value> take_value() &&;
    std::optional<RequestId> request_id() const;

   private:
    friend class TestingGlobalContext;

    Waiter(TypedMessageRouter* typed_message_router,
           const std::optional<std::string>& requester_name);

    void ResolveWithMessageData(Value message_data);
    void ResolveWithRequestPayload(RequestId request_id, Value request_payload);
    void ResolveWithResponsePayload(RequestId request_id,
                                    std::optional<Value> response_payload);

    TypedMessageRouter* const typed_message_router_;
    const std::optional<std::string> requester_name_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool resolved_ = false;
    std::optional<Value> value_;
    std::optional<RequestId> request_id_;
  };

  explicit TestingGlobalContext(TypedMessageRouter* typed_message_router);
  TestingGlobalContext(const TestingGlobalContext&) = delete;
  TestingGlobalContext& operator=(const TestingGlobalContext&) = delete;
  ~TestingGlobalContext();

  // GlobalContext:
  void PostMessageToJs(Value message) override;
  bool IsMainEventLoopThread() const override;
  void ShutDown() override;

  // Allows to configure the result of `IsMainEventLoopThread()` when it's
  // called from the creation thread (on other threads it returns false anyway).
  void set_creation_thread_is_event_loop(bool creation_thread_is_event_loop) {
    creation_thread_is_event_loop_ = creation_thread_is_event_loop;
  }

  // Set a callback to be called whenever a message with the given type is sent
  // to JS.
  void RegisterMessageHandler(const std::string& message_type,
                              MessageCallback callback_to_run);
  // Set a callback to be called whenever a request is sent to JS.
  void RegisterRequestHandler(const std::string& requester_name,
                              RequestCallback callback_to_run);
  // Set a callback to be called whenever a response is sent to JS.
  void RegisterResponseHandler(const std::string& requester_name,
                               ResponseCallback callback_to_run);
  // Handle future requests to `original_requester_name` as if they were sent to
  // `new_requester_name` instead.
  void RegisterRequestRerouter(const std::string& original_requester_name,
                               const std::string& new_requester_name);

  // Returns a waiter for when a message with the specified type arrives.
  std::unique_ptr<Waiter> CreateMessageWaiter(
      const std::string& awaited_message_type);
  // Returns a waiter for when a request message to JS for executing the given
  // function with specified arguments arrives.
  std::unique_ptr<Waiter> CreateRequestWaiter(const std::string& requester_name,
                                              const std::string& function_name,
                                              Value arguments);
  // Returns a waiter for when a response message is sent to JS for the given
  // request.
  std::unique_ptr<Waiter> CreateResponseWaiter(
      const std::string& requester_name,
      RequestId request_id);

  // Sets an expectation that a request will be sent to JS for executing the
  // given function with specified arguments. After this happens, the given
  // reply will be simulated.
  void WillReplyToRequestWith(const std::string& requester_name,
                              const std::string& function_name,
                              Value arguments,
                              Value result_to_reply_with);
  // Same as above, but simulates an error reply.
  void WillReplyToRequestWithError(const std::string& requester_name,
                                   const std::string& function_name,
                                   Value arguments,
                                   const std::string& error_to_reply_with);

 private:
  // Stores the passed callback. It's essentially a union, since exactly one of
  // the fields must be populated.
  struct CallbackStorage {
    MessageCallback message_callback;
    RequestCallback request_callback;
    ResponseCallback response_callback;
  };

  struct Expectation {
    // Filter fields:
    // * The expectation only matches messages with the given
    //   `TypedMessage::type` value.
    std::string awaited_message_type;
    // * If set, the expectation only matches request/response messages with the
    //   given ID.
    std::optional<RequestId> awaited_request_id;
    // * If set, the expectation only matches request messages with the given
    //   `RequestMessageData::payload` value.
    std::optional<Value> awaited_request_payload;

    // The callback to trigger when the expectation is met.
    CallbackStorage callback_storage;

    // Whether the expectation is a one-time.
    bool once = true;
  };

  Expectation MakeRequestExpectation(const std::string& requester_name,
                                     const std::string& function_name,
                                     Value arguments,
                                     RequestCallback callback_to_run);
  void AddExpectation(Expectation expectation);
  std::optional<CallbackStorage> FindMatchingExpectation(
      const std::string& message_type,
      std::optional<RequestId> request_id,
      const Value* request_payload);
  bool HandleMessageToJs(Value message);
  void HandleReroutedRequest(const std::string& new_requester_name,
                             RequestId request_id,
                             Value request_payload);
  void HandleReroutedResponse(
      const std::string& new_requester_name,
      RequestId request_id,
      std::optional<Value> response_payload,
      std::optional<std::string> response_error_message);

  TypedMessageRouter* const typed_message_router_;
  // ID of the thread on which `this` was created.
  const std::thread::id creation_thread_id_;
  // The result to be returned from `IsMainEventLoopThread()` when called on the
  // creation thread.
  bool creation_thread_is_event_loop_ = true;
  std::mutex mutex_;
  std::deque<Expectation> expectations_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_TESTING_GLOBAL_CONTEXT_H_
