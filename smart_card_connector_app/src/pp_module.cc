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

#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/external_logs_printer.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/nacl_io_utils.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
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
        libusb_over_chrome_usb_global_(new LibusbOverChromeUsbGlobal(
            &typed_message_router_, this, pp::Module::Get()->core())),
        pcsc_lite_server_global_(new PcscLiteServerGlobal(this)) {
    typed_message_router_.AddRoute(&external_logs_printer_);

    StartServicesInitialization();
  }

  ~PpInstance() override {
    typed_message_router_.RemoveRoute(&external_logs_printer_);

    // Detach the LibusbNaclGlobal and leak it intentionally, so that any
    // concurrent libusb_* function calls still don't result in UB.
    libusb_over_chrome_usb_global_->Detach();
    libusb_over_chrome_usb_global_.release();
    // Detach the PcscLiteServerGlobal and leak it intentionally to allow
    // graceful shutdown (because of possible concurrent calls).
    pcsc_lite_server_global_->Detach();
    pcsc_lite_server_global_.release();
  }

  void HandleMessage(const pp::Var& message) override {
    if (!typed_message_router_.OnMessageReceived(message)) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected message received: "
                                  << DebugDumpVar(message);
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
        new PcscLiteServerClientsManagementBackend(this,
                                                   &typed_message_router_));

    GOOGLE_SMART_CARD_LOG_DEBUG << "All services are successfully "
                                << "initialized, posting ready message...";
    TypedMessage ready_message;
    ready_message.type = GetPcscLiteServerReadyMessageType();
    ready_message.data = MakePcscLiteServerReadyMessageData();
    PostMessage(MakeVar(ready_message));
  }

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

Module* CreateModule() { return new google_smart_card::PpModule; }

}  // namespace pp
