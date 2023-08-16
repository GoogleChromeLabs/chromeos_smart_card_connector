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

#include <iostream>
#include <string>
#include <utility>

#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include "common/cpp/src/public/global_context_impl_nacl.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_debug_dumping.h"
#include "common/cpp/src/public/value_nacl_pp_var_conversion.h"
#include "common/integration_testing/src/public/integration_test_service.h"

namespace google_smart_card {

namespace {

// The NaCl module implementation that hosts Javascript-and-C++ integration test
// service and helpers.
class IntegrationTestPpInstance final : public pp::Instance {
 public:
  explicit IntegrationTestPpInstance(PP_Instance instance)
      : pp::Instance(instance),
        global_context_(pp::Module::Get()->core(), this) {
    IntegrationTestService::GetInstance()->Activate(&global_context_,
                                                    &typed_message_router_);
  }

  ~IntegrationTestPpInstance() override {
    IntegrationTestService::GetInstance()->Deactivate();
  }

  void HandleMessage(const pp::Var& message) override {
    std::string error_message;
    optional<Value> message_value =
        ConvertPpVarToValue(message, &error_message);
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
  GlobalContextImplNacl global_context_;
  TypedMessageRouter typed_message_router_;
};

class IntegrationTestPpModule final : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) override {
    return new IntegrationTestPpInstance(instance);
  }
};

}  // namespace

}  // namespace google_smart_card

namespace pp {

Module* CreateModule() {
  return new google_smart_card::IntegrationTestPpModule;
}

}  // namespace pp
