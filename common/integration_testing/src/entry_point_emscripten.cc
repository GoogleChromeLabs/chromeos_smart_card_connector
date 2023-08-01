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

#include <google_smart_card_common/global_context_impl_emscripten.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_emscripten_val_conversion.h>
#include <google_smart_card_integration_testing/integration_test_service.h>

namespace google_smart_card {

namespace {

class GoogleSmartCardModule final {
 public:
  explicit GoogleSmartCardModule(emscripten::val post_message_callback)
      : global_context_(std::make_shared<GlobalContextImplEmscripten>(
            std::this_thread::get_id(),
            post_message_callback)) {
    IntegrationTestService::GetInstance()->Activate(global_context_.get(),
                                                    &typed_message_router_);
  }

  GoogleSmartCardModule(const GoogleSmartCardModule&) = delete;
  GoogleSmartCardModule& operator=(const GoogleSmartCardModule&) = delete;

  ~GoogleSmartCardModule() {
    IntegrationTestService::GetInstance()->Deactivate();
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
};

}  // namespace

EMSCRIPTEN_BINDINGS(google_smart_card) {
  emscripten::class_<GoogleSmartCardModule>("GoogleSmartCardModule")
      .constructor<emscripten::val>()
      .function("postMessage", &GoogleSmartCardModule::OnMessageReceivedFromJs);
}

}  // namespace google_smart_card
