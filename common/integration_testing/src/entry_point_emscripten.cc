// Copyright 2023 Google Inc. All Rights Reserved.
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

// This file contains implementation of the Emscripten module entry point to be
// used for JS-to-C++ tests.

#ifndef __EMSCRIPTEN__
#error "This file should only be used in Emscripten builds"
#endif  // __EMSCRIPTEN__

#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "common/cpp/src/public/global_context_impl_emscripten.h"
#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_emscripten_val_conversion.h"
#include <google_smart_card_integration_testing/integration_test_service.h>

namespace google_smart_card {

namespace {

// This class is exposed to the JS counterpart as the entry point that
// sends/receives messages.
//
// Incoming messages (e.g., requests to enable some test helper) are delivered
// to the appropriate handler using `typed_message_router_`. Outgoing messages
// (e.g., responses to the incoming requests) are sent by calling
// `post_message_callback`.
class GoogleSmartCardModule final {
 public:
  explicit GoogleSmartCardModule(emscripten::val post_message_callback)
      : global_context_(std::make_shared<GlobalContextImplEmscripten>(
            std::this_thread::get_id(),
            post_message_callback)) {
    // This service is a small abstraction on top of C++ test helpers: it
    // registers handlers for "SetUp"/"TearDownAll"/"HandleMessage" incoming
    // requests, converting them into corresponding method calls on the helpers.
    //
    // Note: which helpers are available in a given test depends on what gets
    // linked into the final executable: see the example in
    // integration_test_helper.h for how helpers "register" themselves.
    IntegrationTestService::GetInstance()->Activate(global_context_.get(),
                                                    &typed_message_router_);
  }

  GoogleSmartCardModule(const GoogleSmartCardModule&) = delete;
  GoogleSmartCardModule& operator=(const GoogleSmartCardModule&) = delete;

  ~GoogleSmartCardModule() {
    // Stop handling incoming requests and clean up the service state.
    IntegrationTestService::GetInstance()->Deactivate();
  }

  // Triggered whenever the JS side sends a message to us.
  void OnMessageReceivedFromJs(emscripten::val message) {
    std::string error_message;
    optional<Value> message_value =
        ConvertEmscriptenValToValue(message, &error_message);
    if (!message_value) {
      GOOGLE_SMART_CARD_LOG_FATAL
          << "Unexpected JS message received - cannot parse: " << error_message;
    }
    // Parse and route the message to an appropriate C++ handler.
    if (!typed_message_router_.OnMessageReceived(std::move(*message_value),
                                                 &error_message)) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Failure while handling JS message: "
                                  << error_message;
    }
  }

 private:
  // Provides Emscripten-specific operations for toolchain-agnostic code. Stored
  // in a shared pointer because its implementation relies on this.
  std::shared_ptr<GlobalContextImplEmscripten> global_context_;
  // Delivers incoming messages to the previously registered handler. Routing is
  // based on the "type" field (see typed_message.h).
  TypedMessageRouter typed_message_router_;
};

}  // namespace

// Expose the class to the JavaScript side of the test. All communication with
// the JS side happens via `postMessage()` and `post_message_callback`.
EMSCRIPTEN_BINDINGS(google_smart_card) {
  emscripten::class_<GoogleSmartCardModule>("GoogleSmartCardModule")
      .constructor<emscripten::val>()
      .function("postMessage", &GoogleSmartCardModule::OnMessageReceivedFromJs);
}

}  // namespace google_smart_card
