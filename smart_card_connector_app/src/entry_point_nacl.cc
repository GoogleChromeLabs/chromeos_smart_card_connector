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

#include "common/cpp/src/public/external_logs_printer.h"
#include "common/cpp/src/public/global_context_impl_nacl.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/nacl_io_utils.h"
#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_nacl_pp_var_conversion.h"
#include "smart_card_connector_app/src/application.h"

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
            MakeUnique<GlobalContextImplNacl>(pp::Module::Get()->core(), this)),
        application_(MakeUnique<Application>(
            global_context_.get(),
            &typed_message_router_,
            std::bind(&PpInstance::InitializeOnBackgroundThread, this))) {
    typed_message_router_.AddRoute(&external_logs_printer_);
  }

  ~PpInstance() override {
    typed_message_router_.RemoveRoute(&external_logs_printer_);

    // Intentionally leak the `Application` and `GlobalContext` objects as they
    // might still be used by background threads. Only shut down the objects
    // (which prevents them from referring to us and from talking to the
    // JavaScript side).
    application_->ShutDownAndWait();
    (void)application_.release();
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
  void InitializeOnBackgroundThread() {
    InitializeNaclIo(*this);
    MountNaclIoFolders();
  }

  std::unique_ptr<GlobalContextImplNacl> global_context_;
  TypedMessageRouter typed_message_router_;
  std::unique_ptr<Application> application_;
  ExternalLogsPrinter external_logs_printer_{kJsLogsHandlerMessageType};
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
