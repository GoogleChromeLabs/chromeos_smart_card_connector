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

#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/external_logs_printer.h>
#include <google_smart_card_common/global_context_impl_nacl.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/nacl_io_utils.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>
#include <google_smart_card_libusb/global.h>
#include <google_smart_card_pcsc_lite_server/global.h>
#include <google_smart_card_pcsc_lite_server_clients_management/backend.h>
#include <google_smart_card_pcsc_lite_server_clients_management/ready_message.h>

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
        libusb_over_chrome_usb_global_(
            MakeUnique<LibusbOverChromeUsbGlobal>(global_context_.get(),
                                                  &typed_message_router_)),
        pcsc_lite_server_global_(new PcscLiteServerGlobal(this)) {
    typed_message_router_.AddRoute(&external_logs_printer_);

    StartServicesInitialization();
  }

  ~PpInstance() override {
    typed_message_router_.RemoveRoute(&external_logs_printer_);

    // Intentionally leak objects that might still be used by background
    // threads. Only detach them from `this` and the JavaScript side.
    global_context_->DisableJsCommunication();
    global_context_.release();
    libusb_over_chrome_usb_global_->Detach();
    libusb_over_chrome_usb_global_.release();
    pcsc_lite_server_global_.release();
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
  void StartServicesInitialization() {
    std::thread(&PpInstance::InitializeServices, this).detach();
  }

  void InitializeServices() {
    GOOGLE_SMART_CARD_LOG_DEBUG << "Performing services initialization...";

    InitializeNaclIo(*this);
    pcsc_lite_server_global_->InitializeAndRunDaemonThread();

    pcsc_lite_server_clients_management_backend_.reset(
        new PcscLiteServerClientsManagementBackend(global_context_.get(),
                                                   &typed_message_router_));

    GOOGLE_SMART_CARD_LOG_DEBUG << "All services are successfully "
                                << "initialized, posting ready message...";
    TypedMessage ready_message;
    ready_message.type = GetPcscLiteServerReadyMessageType();
    // TODO: Directly create `Value` instead of transforming from `pp::Var`.
    ready_message.data =
        ConvertPpVarToValueOrDie(MakePcscLiteServerReadyMessageData());
    Value ready_message_value = ConvertToValueOrDie(std::move(ready_message));
    // TODO: Directly post `Value` instead of `pp::Var`.
    PostMessage(ConvertValueToPpVar(ready_message_value));
  }

  std::unique_ptr<GlobalContextImplNacl> global_context_;
  TypedMessageRouter typed_message_router_;
  ExternalLogsPrinter external_logs_printer_{kJsLogsHandlerMessageType};
  std::unique_ptr<LibusbOverChromeUsbGlobal> libusb_over_chrome_usb_global_;
  std::unique_ptr<PcscLiteServerClientsManagementBackend>
      pcsc_lite_server_clients_management_backend_;
  std::unique_ptr<PcscLiteServerGlobal> pcsc_lite_server_global_;
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
