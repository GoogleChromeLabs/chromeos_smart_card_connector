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

#include "smart_card_connector_app/src/application.h"

#include <functional>
#include <thread>
#include <utility>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "third_party/libusb/webport/src/public/libusb_web_port_service.h"
#include "third_party/pcsc-lite/naclport/server/src/public/pcsc_lite_server_web_port_service.h"
#include "third_party/pcsc-lite/naclport/server_clients_management/src/google_smart_card_pcsc_lite_server_clients_management/backend.h"
#include "third_party/pcsc-lite/naclport/server_clients_management/src/google_smart_card_pcsc_lite_server_clients_management/ready_message.h"

namespace google_smart_card {

Application::Application(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router,
    std::function<void()> background_initialization_callback)
    : global_context_(global_context),
      typed_message_router_(typed_message_router),
      background_initialization_callback_(background_initialization_callback),
      libusb_web_port_service_(
          MakeUnique<LibusbWebPortService>(global_context_,
                                           typed_message_router_)),
      pcsc_lite_server_web_port_service_(
          MakeUnique<PcscLiteServerWebPortService>(
              global_context_,
              libusb_web_port_service_.get())) {
  ScheduleServicesInitialization();
}

void Application::ShutDownAndWait() {
  pcsc_lite_server_clients_management_backend_.reset();
  pcsc_lite_server_web_port_service_->ShutDownAndWait();
  libusb_web_port_service_->ShutDown();
}

Application::~Application() = default;

void Application::ScheduleServicesInitialization() {
  // TODO(emaxx): Ensure the correct lifetime of `this`.
  std::thread(&Application::InitializeServicesOnBackgroundThread, this)
      .detach();
}

void Application::InitializeServicesOnBackgroundThread() {
  GOOGLE_SMART_CARD_LOG_DEBUG << "Performing services initialization...";

  if (background_initialization_callback_)
    background_initialization_callback_();
  pcsc_lite_server_web_port_service_->InitializeAndRunDaemonThread();

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
