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

// This file contains the application's entry point that is used in Emscripten
// builds. It performs the necessary initialization and then instantiates the
// Application class, which implements the actual functionality of the
// PC/SC-Lite daemon.

#ifndef __EMSCRIPTEN__
#error "This file should only be used in Emscripten builds"
#endif  // __EMSCRIPTEN__

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "common/cpp/src/public/global_context_impl_emscripten.h"
#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_emscripten_val_conversion.h"
#include "smart_card_connector_app/src/application.h"

namespace google_smart_card {

namespace {

// A class that is instantiated by the JavaScript code in order to start the
// application and for exchanging messages with it.
class GoogleSmartCardModule final {
 public:
  explicit GoogleSmartCardModule(emscripten::val post_message_callback)
      : global_context_(std::make_shared<GlobalContextImplEmscripten>(
            std::this_thread::get_id(),
            post_message_callback)),
        application_(MakeUnique<Application>(
            global_context_.get(),
            &typed_message_router_,
            /*background_initialization_callback=*/std::function<void()>())) {}

  GoogleSmartCardModule(const GoogleSmartCardModule&) = delete;
  GoogleSmartCardModule& operator=(const GoogleSmartCardModule&) = delete;

  ~GoogleSmartCardModule() {
    // Intentionally leak the `Application` and `GlobalContext` objects as they
    // might still be used by background threads. Only shut down the objects
    // (which prevents them from referring to us and from talking to the
    // JavaScript side).
    application_->ShutDownAndWait();
    (void)application_.release();
    global_context_->ShutDown();
    new std::shared_ptr<GlobalContextImplEmscripten>(global_context_);
  }

  void OnMessageReceivedFromJs(emscripten::val message) {
    std::string error_message;
    optional<Value> message_value =
        ConvertEmscriptenValToValue(message, &error_message);
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
  std::shared_ptr<GlobalContextImplEmscripten> global_context_;
  TypedMessageRouter typed_message_router_;
  std::unique_ptr<Application> application_;
};

}  // namespace

// Expose using the Emscripten's Embind functionality the
// `GoogleSmartCardModule` class to JavaScript code.
EMSCRIPTEN_BINDINGS(google_smart_card) {
  emscripten::class_<GoogleSmartCardModule>("GoogleSmartCardModule")
      .constructor<emscripten::val>()
      .function("postMessage", &GoogleSmartCardModule::OnMessageReceivedFromJs);
}

}  // namespace google_smart_card
