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

#ifndef SMART_CARD_CONNECTOR_APP_APPLICATION_H_
#define SMART_CARD_CONNECTOR_APP_APPLICATION_H_

#include <functional>
#include <memory>

#include <google_smart_card_common/admin_policy_getter.h>
#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_pcsc_lite_server_clients_management/backend.h>

#include "third_party/libusb/webport/src/public/libusb_web_port_service.h"
#include "third_party/pcsc-lite/naclport/server/src/public/pcsc_lite_server_web_port_service.h"

namespace google_smart_card {

// The core class of the application. This class initializes and runs the
// PC/SC-Lite daemon, handler of client requests and other related
// functionality.
//
// This class' interface is toolchain-independent; it's used by
// toolchain-specific entry point implementations (the Emscripten and the Native
// Client ones).
class Application final {
 public:
  // Initializes and starts the application. The `typed_message_router` argument
  // is used for subscribing to messages received from the JavaScript side. The
  // `background_initialization_callback` callback will be executed on a
  // background thread before any other initialization.
  //
  // Both `global_context` and `typed_message_router` must outlive `this`.
  Application(GlobalContext* global_context,
              TypedMessageRouter* typed_message_router,
              std::function<void()> background_initialization_callback);

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  // Note that the destructor is not guaranteed to be called, as the framework
  // used to run the executable may end it instantly.
  ~Application();

  // Must be called before destroying the object.
  // Shuts down the application's daemon threads and stops using
  // `global_context` and `typed_message_router`.
  void ShutDownAndWait();

 private:
  void ScheduleServicesInitialization();
  void InitializeServicesOnBackgroundThread();

  GlobalContext* const global_context_;
  TypedMessageRouter* const typed_message_router_;
  AdminPolicyGetter admin_policy_getter_;
  std::function<void()> background_initialization_callback_;
  std::unique_ptr<LibusbWebPortService> libusb_web_port_service_;
  std::unique_ptr<PcscLiteServerClientsManagementBackend>
      pcsc_lite_server_clients_management_backend_;
  std::unique_ptr<PcscLiteServerWebPortService>
      pcsc_lite_server_web_port_service_;
};

}  // namespace google_smart_card

#endif  // SMART_CARD_CONNECTOR_APP_APPLICATION_H_
