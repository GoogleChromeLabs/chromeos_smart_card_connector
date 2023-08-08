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

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_debug_dumping.h"
#include <google_smart_card_pcsc_lite_client/global.h>
#include <google_smart_card_pcsc_lite_cpp_demo/demo.h>

#include "built_in_pin_dialog/built_in_pin_dialog_server.h"
#include "chrome_certificate_provider/api_bridge.h"
#include "chrome_certificate_provider/types.h"
#include "ui_bridge.h"

namespace scc = smart_card_client;
namespace ccp = scc::chrome_certificate_provider;
namespace gsc = google_smart_card;

namespace smart_card_client {

namespace {

// Collects all currently available certificates and returns them through the
// |certificates| output argument.
void GetCertificates(std::vector<ccp::ClientCertificateInfo>* certificates) {
  //
  // CHANGE HERE:
  // Place your custom code here:
  //

  // Note: the bytes "1, 2, 3" and the signature algorithms below are just
  // an example. In the real application, replace them with the bytes of the
  // DER encoding of a X.509 certificate and the supported algorithms.
  ccp::ClientCertificateInfo certificate_info_1;
  certificate_info_1.certificate.assign({1, 2, 3});
  certificate_info_1.supported_algorithms.push_back(
      ccp::Algorithm::kRsassaPkcs1v15Sha1);
  ccp::ClientCertificateInfo certificate_info_2;
  certificate_info_2.supported_algorithms.push_back(
      ccp::Algorithm::kRsassaPkcs1v15Sha512);
  certificates->push_back(certificate_info_1);
  certificates->push_back(certificate_info_2);
}

// Reports all currently available certificates to Chrome via the
// chrome.certificateProvider.setCertificates API. Returns false if this failed.
bool ReportAvailableCertificates(
    std::weak_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge) {
  const std::shared_ptr<ccp::ApiBridge>
      locked_chrome_certificate_provider_api_bridge =
          chrome_certificate_provider_api_bridge.lock();
  if (!locked_chrome_certificate_provider_api_bridge) {
    GOOGLE_SMART_CARD_LOG_INFO << "Cannot provide certificates: The "
                               << "shutdown process has started";
    return false;
  }

  std::vector<ccp::ClientCertificateInfo> certificates;
  GetCertificates(&certificates);
  locked_chrome_certificate_provider_api_bridge->SetCertificates(certificates);
  return true;
}

// This method is executed on a background thread after all of the
// initialization steps finish.
void WorkOnBackgroundThread(
    std::weak_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge) {
  //
  // CHANGE HERE:
  // Place your custom initialization code here:
  //

  // Report the currently available list of certificates after the
  // initialization is done and all available certificates are known, as per the
  // requirements imposed by Chrome - see
  // <https://developer.chrome.com/extensions/certificateProvider#method-setCertificates>.
  ReportAvailableCertificates(chrome_certificate_provider_api_bridge);
}

}  // namespace

class Application::ClientCertificatesRequestHandler final
    : public ccp::CertificatesRequestHandler {
 public:
  // Handles the received certificates request.
  //
  // Returns whether the operation finished successfully. In case of success,
  // the resulting certificates information should be returned through the
  // |result| output argument.
  //
  // Note that this method is executed by
  // chrome_certificate_provider::ApiBridge object on a separate background
  // thread. Multiple requests can be executed simultaneously (they will run
  // in different background threads).
  bool HandleRequest(std::vector<ccp::ClientCertificateInfo>* result) override {
    GetCertificates(result);
    return true;
  }
};

class Application::ClientSignatureRequestHandler final
    : public ccp::SignatureRequestHandler {
 public:
  explicit ClientSignatureRequestHandler(
      std::weak_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge)
      : chrome_certificate_provider_api_bridge_(
            chrome_certificate_provider_api_bridge) {}

  // Handles the received signature request (the request data is passed
  // through the |signature_request| argument).
  //
  // Returns whether the operation finished successfully. In case of success,
  // the resulting signature should be returned through the |result| output
  // argument.
  //
  // Note that this method is executed by
  // chrome_certificate_provider::ApiBridge object on a separate background
  // thread. Multiple requests can be executed simultaneously (they will run
  // in different background threads).
  bool HandleRequest(const ccp::SignatureRequest& signature_request,
                     std::vector<uint8_t>* result) override {
    //
    // CHANGE HERE:
    // Place your custom code here:
    //

    const std::shared_ptr<ccp::ApiBridge>
        locked_chrome_certificate_provider_api_bridge =
            chrome_certificate_provider_api_bridge_.lock();
    if (!locked_chrome_certificate_provider_api_bridge) {
      GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] Skipped PIN dialog "
                                 << "demo: the shutdown process has started";
      return false;
    }

    GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] Running PIN dialog "
                               << "demo...";
    ccp::RequestPinOptions request_pin_options;
    request_pin_options.sign_request_id = signature_request.sign_request_id;
    std::string pin;
    if (!locked_chrome_certificate_provider_api_bridge->RequestPin(
            request_pin_options, &pin)) {
      GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] demo finished: "
                                 << "dialog was canceled.";
      return false;
    }

    ccp::StopPinRequestOptions stop_pin_request_options;
    stop_pin_request_options.sign_request_id =
        signature_request.sign_request_id;
    locked_chrome_certificate_provider_api_bridge->StopPinRequest(
        stop_pin_request_options);

    GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] demo finished: "
                               << "received PIN of length " << pin.length()
                               << " entered by the user.";

    // Note: these bytes "4, 5, 6" below are just an example. In the real
    // application, replace them with the bytes of the real signature
    // generated by the smart card.
    *result = std::vector<uint8_t>({4, 5, 6});

    return true;
  }

 private:
  const std::weak_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge_;
};

class Application::ClientMessageFromUiHandler final
    : public MessageFromUiHandler {
 public:
  explicit ClientMessageFromUiHandler(
      std::weak_ptr<UiBridge> ui_bridge,
      std::weak_ptr<BuiltInPinDialogServer> built_in_pin_dialog_server)
      : ui_bridge_(ui_bridge),
        built_in_pin_dialog_server_(built_in_pin_dialog_server) {}

  void HandleMessageFromUi(gsc::Value message) override {
    //
    // CHANGE HERE:
    // Place your custom code here:
    //

    if (message.is_dictionary()) {
      const gsc::Value* const command_value =
          message.GetDictionaryItem("command");
      if (command_value && command_value->is_string()) {
        const std::string& command = command_value->GetString();
        if (command == "run_test") {
          OnRunTestCommandReceived();
          return;
        }
      }
    }
    GOOGLE_SMART_CARD_LOG_ERROR << "Unexpected message from UI: "
                                << gsc::DebugDumpValueSanitized(message);
  }

 private:
  void OnRunTestCommandReceived() {
    // Demo code for the built-in PIN dialog.
    //
    // Note: This built-in PIN dialog should only be used for the PIN requests
    // that aren't associated with signature requests made by Chrome, since
    // for those the locked_chrome_certificate_provider_api_bridge's
    // RequestPin() should be used instead (see the example in HandleRequest()
    // above).
    GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] Running built-in PIN "
                               << "dialog demo...";
    const std::shared_ptr<BuiltInPinDialogServer> locked_pin_dialog_server =
        built_in_pin_dialog_server_.lock();
    if (!locked_pin_dialog_server) {
      GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] Skipped PIN dialog "
                                 << "demo: the shutdown process has started";
      return;
    }
    std::string pin;
    if (locked_pin_dialog_server->RequestPin(&pin)) {
      GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] received PIN of "
                                 << "length " << pin.length()
                                 << " entered by the user.";
    } else {
      GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] PIN dialog was "
                                 << "canceled.";
    }

    GOOGLE_SMART_CARD_LOG_INFO << "[PC/SC-Lite DEMO] Starting PC/SC-Lite "
                               << "demo...";
    SendOutputMessageToUi("Starting demo...");
    if (gsc::ExecutePcscLiteCppDemo()) {
      GOOGLE_SMART_CARD_LOG_INFO << "[PC/SC-Lite DEMO] demo finished "
                                 << "successfully.";
      SendOutputMessageToUi("Demo finished successfully.");
    } else {
      GOOGLE_SMART_CARD_LOG_ERROR << "[PC/SC-Lite DEMO] demo failed.";
      SendOutputMessageToUi("Demo failed.");
    }
  }

  void SendOutputMessageToUi(const std::string& text) {
    std::shared_ptr<UiBridge> locked_ui_bridge = ui_bridge_.lock();
    if (!locked_ui_bridge)
      return;
    gsc::Value message(gsc::Value::Type::kDictionary);
    message.SetDictionaryItem("output_message", text);
    locked_ui_bridge->SendMessageToUi(std::move(message));
  }

  std::weak_ptr<UiBridge> ui_bridge_;
  const std::weak_ptr<BuiltInPinDialogServer> built_in_pin_dialog_server_;
};

Application::Application(gsc::GlobalContext* global_context,
                         gsc::TypedMessageRouter* typed_message_router)
    : request_handling_mutex_(std::make_shared<std::mutex>()),
      pcsc_lite_over_requester_global_(
          new gsc::PcscLiteOverRequesterGlobal(global_context,
                                               typed_message_router)),
      built_in_pin_dialog_server_(
          new BuiltInPinDialogServer(global_context, typed_message_router)),
      chrome_certificate_provider_api_bridge_(
          new ccp::ApiBridge(global_context,
                             typed_message_router,
                             request_handling_mutex_)),
      ui_bridge_(new UiBridge(global_context,
                              typed_message_router,
                              request_handling_mutex_)),
      certificates_request_handler_(new ClientCertificatesRequestHandler),
      signature_request_handler_(new ClientSignatureRequestHandler(
          chrome_certificate_provider_api_bridge_)),
      message_from_ui_handler_(
          new ClientMessageFromUiHandler(ui_bridge_,
                                         built_in_pin_dialog_server_)) {
  chrome_certificate_provider_api_bridge_->SetCertificatesRequestHandler(
      certificates_request_handler_);
  chrome_certificate_provider_api_bridge_->SetSignatureRequestHandler(
      signature_request_handler_);
  ui_bridge_->SetHandler(message_from_ui_handler_);

  std::thread(&WorkOnBackgroundThread, chrome_certificate_provider_api_bridge_)
      .detach();
}

Application::~Application() {
  // Intentionally leak `pcsc_lite_over_requester_global_` without destroying
  // it, because there might still be background threads that access it.
  pcsc_lite_over_requester_global_->ShutDown();
  (void)pcsc_lite_over_requester_global_.release();

  built_in_pin_dialog_server_->ShutDown();
  chrome_certificate_provider_api_bridge_->ShutDown();
  ui_bridge_->ShutDown();
}

}  // namespace smart_card_client
