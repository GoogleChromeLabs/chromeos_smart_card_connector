// Copyright 2016 Google Inc. All Rights Reserved.
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

// This file contains the application's entry point that is used in Native
// Client builds. It performs the necessary initialization and then instantiates
// the Application class, which implements the actual functionality of the
// PC/SC-Lite daemon.

#ifndef __native_client__
#error "This file should only be used in Native Client builds"
#endif  // __native_client__

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/external_logs_printer.h>
#include <google_smart_card_common/global_context_impl_nacl.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/nacl_io_utils.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

#include "application.h"

namespace google_smart_card {

namespace {

// Message type of the messages containing logs forwarded from the JS side.
// This constant must match the one in background.js.
constexpr char kJsLogsHandlerMessageType[] = "js_logs_handler";

class PpInstance final : public pp::Instance {
 public:
  explicit PpInstance(PP_Instance instance)
      : pp::Instance(instance),
        global_context_(
            MakeUnique<GlobalContextImplNacl>(pp::Module::Get()->core(),
                                              this)) {
    typed_message_router_.AddRoute(&external_logs_printer_);
  }

  ~PpInstance() override {
    typed_message_router_.RemoveRoute(&external_logs_printer_);

    // Intentionally leak the global context as it might still be used by
    // background threads. Only shut it down (which prevents it from referring
    // to us and from talking to the JavaScript side).
    global_context_->ShutDown();
    (void)global_context_.release();
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
  void InitializeOnBackgroundThread() { InitializeNaclIo(*this); }

  std::unique_ptr<GlobalContextImplNacl> global_context_;
  TypedMessageRouter typed_message_router_;
  ExternalLogsPrinter external_logs_printer_{kJsLogsHandlerMessageType};
  Application application_{
      global_context_.get(), &typed_message_router_,
      std::bind(&PpInstance::InitializeOnBackgroundThread, this)};
};

class PpModule final : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) override {
    return new PpInstance(instance);
  }
};

}  // namespace

}  // namespace google_smart_card

namespace pp {

Module* CreateModule() {
  return new google_smart_card::PpModule;
}

}  // namespace pp
