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

#ifndef GOOGLE_SMART_CARD_COMMON_TESTING_GLOBAL_CONTEXT_H_
#define GOOGLE_SMART_CARD_COMMON_TESTING_GLOBAL_CONTEXT_H_

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Test implementation of `GlobalContext` that allows to set up expectations on
// the messages sent to the JavaScript side and to simulate responses from it.
class TestingGlobalContext final : public GlobalContext {
 public:
  // Callback to be run when an expected message is sent to JS. For
  // request/response messages, the parameters are `RequestMessageData::payload`
  // and `RequestMessageData::request_id`. For other messages, only
  // `TypedMessage::data` is passed.
  using Callback = std::function<void(optional<Value>, optional<RequestId>)>;

  // Helper returned by `Create...Waiter()` methods. Allows to wait until the
  // specified C++-to-JS message is sent.
  class Waiter final {
   public:
    Waiter(const Waiter&) = delete;
    Waiter& operator=(const Waiter&) = delete;
    ~Waiter();

    void Wait();
    void Reply(Value result_to_reply_with);

    const optional<Value>& value() const;
    optional<Value> take_value() &&;
    optional<RequestId> request_id() const;

   private:
    friend class TestingGlobalContext;

    Waiter(TypedMessageRouter* typed_message_router,
           const optional<std::string>& requester_name);

    void Resolve(optional<Value> value, optional<RequestId> request_id);

    TypedMessageRouter* const typed_message_router_;
    const optional<std::string> requester_name_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool resolved_ = false;
    optional<Value> value_;
    optional<RequestId> request_id_;
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
                              Callback callback_to_run);
  // Set a callback to be called whenever a request is sent to JS.
  void RegisterRequestHandler(const std::string& requester_name,
                              Callback callback_to_run);

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
  struct Expectation {
    // Filter fields:
    // * The expectation only matches messages with the given
    //   `TypedMessage::type` value.
    std::string awaited_message_type;
    // * If set, the expectation only matches request/response messages with the
    //   given ID.
    optional<RequestId> awaited_request_id;
    // * If set, the expectation only matches request messages with the given
    //   `RequestMessageData::payload` value.
    optional<Value> awaited_request_payload;

    // The callback to trigger when the expectation is met.
    Callback callback_to_run;

    // Whether the expectation is a one-time.
    bool once = true;
  };

  Expectation MakeRequestExpectation(const std::string& requester_name,
                                     const std::string& function_name,
                                     Value arguments,
                                     Callback callback_to_run);
  void AddExpectation(Expectation expectation);
  optional<Callback> FindMatchingExpectation(const std::string& message_type,
                                             optional<RequestId> request_id,
                                             const Value* request_payload);
  bool HandleMessageToJs(Value message);

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

#endif  // GOOGLE_SMART_CARD_COMMON_TESTING_GLOBAL_CONTEXT_H_
