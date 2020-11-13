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

#include <memory>
#include <mutex>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_pcsc_lite_client/global.h>

#include "built_in_pin_dialog/built_in_pin_dialog_server.h"
#include "chrome_certificate_provider/api_bridge.h"
#include "ui_bridge.h"

namespace smart_card_client {

// The implementation presented here is a skeleton that initializes all pieces
// necessary for PC/SC-Lite client API initialization,
// chrome.certificateProvider JavaScript API integration and the built-in PIN
// dialog integration.
//
// As an example, this implementation starts a background thread running the
// Work method after the initialization happens.
//
// Please note that all blocking operations (for example, PC/SC-Lite API calls
// or PIN requests) should be never executed on the main event loop thread. This
// is because all communication with the JavaScript side works through
// exchanging of messages between the executable module and JavaScript side, and
// the incoming messages are received and routed on the main thread. Actually,
// most of the blocking operations implemented in this code contain assertions
// that they are not called on the main thread.
class Application final {
 public:
  // Initializes and starts the application. The `typed_message_router` argument
  // is used for subscribing to messages received from the JavaScript side.
  //
  // Both `global_context` and `typed_message_router` must outlive `this`.
  Application(google_smart_card::GlobalContext* global_context,
              google_smart_card::TypedMessageRouter* typed_message_router);

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  // Note that the destructor is not guaranteed to be called, as the framework
  // using for running the executable may terminate it instantly.
  ~Application();

 private:
  // Mutex that enforces that all requests are handled sequentially.
  const std::shared_ptr<std::mutex> request_handling_mutex_;
  // Object that initializes the global common state used by the PC/SC-Lite
  // client API functions.
  //
  // The stored pointer is leaked intentionally in the class destructor - see
  // its comment for the justification.
  std::unique_ptr<google_smart_card::PcscLiteOverRequesterGlobal>
      pcsc_lite_over_requester_global_;
  // Object that allows to perform built-in PIN dialog requests.
  std::shared_ptr<BuiltInPinDialogServer> built_in_pin_dialog_server_;
  // Object that allows to make calls to and receive events from the
  // chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#events>).
  std::shared_ptr<chrome_certificate_provider::ApiBridge>
      chrome_certificate_provider_api_bridge_;
  // Object that sends/receives messages to/from the UI.
  const std::shared_ptr<UiBridge> ui_bridge_;

  // Handler of the onCertificateUpdatesRequested/onCertificatesRequested
  // requests from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesUpdateRequested>
  // and
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>).
  class ClientCertificatesRequestHandler;
  const std::shared_ptr<ClientCertificatesRequestHandler>
      certificates_request_handler_;

  // Handler of the onSignatureRequested/onSignDigestRequested requests
  // from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignatureRequested>
  // and
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>).
  class ClientSignatureRequestHandler;
  const std::shared_ptr<ClientSignatureRequestHandler>
      signature_request_handler_;

  // Handler of messages from UI.
  class ClientMessageFromUiHandler;
  const std::shared_ptr<ClientMessageFromUiHandler> message_from_ui_handler_;
};

}  // namespace smart_card_client
