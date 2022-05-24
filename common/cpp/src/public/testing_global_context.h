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

#include <deque>
#include <mutex>
#include <string>

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
  explicit TestingGlobalContext(TypedMessageRouter* typed_message_router);
  TestingGlobalContext(const TestingGlobalContext&) = delete;
  TestingGlobalContext& operator=(const TestingGlobalContext&) = delete;
  ~TestingGlobalContext();

  // GlobalContext:
  void PostMessageToJs(Value message) override;
  bool IsMainEventLoopThread() const override;
  void ShutDown() override;

  // Allows to configure the result of `IsMainEventLoopThread()`.
  void set_creation_thread_is_event_loop(bool creation_thread_is_event_loop) {
    creation_thread_is_event_loop_ = creation_thread_is_event_loop;
  }

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
    std::string requester_name;
    Value awaited_request_payload;
    optional<Value> payload_to_reply_with;
    optional<std::string> error_to_reply_with;
  };

  static Expectation MakeRequestExpectation(
      const std::string& requester_name,
      const std::string& function_name,
      Value arguments,
      optional<Value> payload_to_reply_with,
      const optional<std::string>& error_to_reply_with);
  void AddExpectation(Expectation expectation);
  optional<Expectation> PopMatchingExpectation(const std::string& message_type,
                                               const Value& request_payload);
  bool HandleMessageToJs(Value message);
  void PostFakeJsReply(RequestId request_id,
                       const std::string& requester_name,
                       optional<Value> payload_to_reply_with,
                       const optional<std::string>& error_to_reply_with);

  TypedMessageRouter* const typed_message_router_;
  bool creation_thread_is_event_loop_ = true;
  std::mutex mutex_;
  std::deque<Expectation> expectations_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_TESTING_GLOBAL_CONTEXT_H_
