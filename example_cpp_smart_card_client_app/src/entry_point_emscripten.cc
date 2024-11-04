// Copyright 2020 Google Inc. All Rights Reserved.
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

// This file contains implementation of the Emscripten module entry point - the
// GoogleSmartCardModule class that is created by the JavaScript code via the
// Emscripten's Embind functionality.

#ifndef __EMSCRIPTEN__
#error "This file should only be used in Emscripten builds"
#endif  // __EMSCRIPTEN__

#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "common/cpp/src/public/global_context_impl_emscripten.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_debug_dumping.h"
#include "common/cpp/src/public/value_emscripten_val_conversion.h"

#include "application.h"

namespace gsc = google_smart_card;

namespace smart_card_client {

namespace {

// A class that is instantiated by the JavaScript code in order to start the
// application and for exchanging messages with it.
class GoogleSmartCardModule final {
 public:
  explicit GoogleSmartCardModule(emscripten::val post_message_callback)
      : global_context_(std::make_shared<gsc::GlobalContextImplEmscripten>(
            std::this_thread::get_id(),
            post_message_callback)),
        application_(global_context_.get(), &typed_message_router_) {}

  GoogleSmartCardModule(const GoogleSmartCardModule&) = delete;
  GoogleSmartCardModule& operator=(const GoogleSmartCardModule&) = delete;

  ~GoogleSmartCardModule() {
    // Intentionally leak `global_context_` without destroying it, because there
    // might still be background threads that access it.
    global_context_->ShutDown();
    new std::shared_ptr<gsc::GlobalContextImplEmscripten>(global_context_);
  }

  void OnMessageReceivedFromJs(emscripten::val message) {
    std::string error_message;
    std::optional<gsc::Value> message_value =
        gsc::ConvertEmscriptenValToValue(message, &error_message);
    if (!message_value) {
      GOOGLE_SMART_CARD_LOG_FATAL
          << "Unexpected JS message received - cannot parse: " << error_message;
    }
    if (!typed_message_router_.OnMessageReceived(std::move(*message_value),
                                                 &error_message)) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Failure while handling JS message: "
                                  << error_message;
    }
  }

 private:
  // Global context that proxies webport-specific operations.
  //
  // The stored pointer is leaked intentionally in the class destructor - see
  // its comment for the justification.
  std::shared_ptr<gsc::GlobalContextImplEmscripten> global_context_;
  // Router of the incoming typed messages that passes incoming messages to the
  // appropriate handlers according the the special type field of the message
  // (see common/cpp/src/public/messaging/typed_message.h).
  gsc::TypedMessageRouter typed_message_router_;
  // The core application functionality that is toolchain-independent.
  Application application_;
};

}  // namespace

// Expose using the Emscripten's Embind functionality the
// `GoogleSmartCardModule` class to JavaScript code.
EMSCRIPTEN_BINDINGS(google_smart_card) {
  emscripten::class_<GoogleSmartCardModule>("GoogleSmartCardModule")
      .constructor<emscripten::val>()
      .function("postMessage", &GoogleSmartCardModule::OnMessageReceivedFromJs);
}

}  // namespace smart_card_client
