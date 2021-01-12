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

#include "application.h"

#include <functional>
#include <thread>
#include <utility>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_libusb/global.h>
#include <google_smart_card_pcsc_lite_server/global.h>
#include <google_smart_card_pcsc_lite_server_clients_management/backend.h>
#include <google_smart_card_pcsc_lite_server_clients_management/ready_message.h>

namespace google_smart_card {

Application::Application(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router,
    std::function<void()> background_initialization_callback)
    : global_context_(global_context),
      typed_message_router_(typed_message_router),
      background_initialization_callback_(background_initialization_callback),
      libusb_over_chrome_usb_global_(
          MakeUnique<LibusbOverChromeUsbGlobal>(global_context_,
                                                typed_message_router_)),
      pcsc_lite_server_global_(
          MakeUnique<PcscLiteServerGlobal>(global_context_)) {
  ScheduleServicesInitialization();
}

Application::~Application() {
  // Intentionally leak objects that might still be used by background threads.
  // Only detach them from `this` and the JavaScript side.
  libusb_over_chrome_usb_global_->Detach();
  (void)libusb_over_chrome_usb_global_.release();
  (void)pcsc_lite_server_global_.release();
}

void Application::ScheduleServicesInitialization() {
  // TODO(emaxx): Ensure the correct lifetime of `this`.
  std::thread(&Application::InitializeServicesOnBackgroundThread, this)
      .detach();
}

void Application::InitializeServicesOnBackgroundThread() {
  GOOGLE_SMART_CARD_LOG_DEBUG << "Performing services initialization...";

  if (background_initialization_callback_)
    background_initialization_callback_();
  pcsc_lite_server_global_->InitializeAndRunDaemonThread();

  pcsc_lite_server_clients_management_backend_ =
      MakeUnique<PcscLiteServerClientsManagementBackend>(global_context_,
                                                         typed_message_router_);

  GOOGLE_SMART_CARD_LOG_DEBUG << "All services are successfully "
                              << "initialized, posting ready message...";
  TypedMessage ready_message;
  ready_message.type = GetPcscLiteServerReadyMessageType();
  ready_message.data = MakePcscLiteServerReadyMessageData();
  global_context_->PostMessageToJs(
      ConvertToValueOrDie(std::move(ready_message)));
}

}  // namespace google_smart_card
