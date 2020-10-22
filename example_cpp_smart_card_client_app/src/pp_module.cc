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

// This file contains implementation of the NaCl module entry point
// pp::CreateModule, which creates an instance of class inherited from the
// pp::Instance class (see
// <https://developer.chrome.com/native-client/devguide/coding/application-structure#native-client-modules-a-closer-look>
// for reference).

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>
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

// This class contains the actual NaCl module implementation.
//
// The implementation presented here is a skeleton that initializes all pieces
// necessary for PC/SC-Lite client API initialization,
// chrome.certificateProvider JavaScript API integration and the built-in PIN
// dialog integration.
//
// As an example, this implementation starts a background thread running the
// Work method after the initialization happens.
//
// Please note that all blocking operations (for example, PC/SC-Lite API calls
// or PIN requests) should be never executed on the main thread. This is because
// all communication with the JavaScript side works through exchanging of
// messages between NaCl module and JavaScript side, and the incoming messages
// are passed by the NaCl framework to the HandleMessage method of this class
// always on the main thread (see
// <https://developer.chrome.com/native-client/devguide/coding/message-system>).
// Actually, most of the blocking operations implemented in this code contain
// assertions that they are not called on the main thread.
class PpInstance final : public pp::Instance {
 public:
  // The constructor that is executed during the NaCl module startup.
  //
  // The implementation presented here does the following:
  // * creates a google_smart_card::TypedMessageRouter class instance that will
  //   be used for handling messages received from the JavaScript side (see the
  //   HandleMessage method);
  // * creates a google_smart_card::PcscLiteClientNaclGlobal class instance that
  //   initializes the internal state required for PC/SC-Lite API functions
  //   implementation;
  // * creates a chrome_certificate_provider::ApiBridge class instance that can
  //   be used to handle requests received from the chrome.certificateProvider
  //   JavaScript API event listeners (see
  //   <https://developer.chrome.com/extensions/certificateProvider#events>);
  // * creates a BuiltInPinDialogServer class instance that allows to perform
  //   built-in PIN dialog requests (for the cases when the
  //   chrome.certificateProvider.requestPin() API cannot be used).
  explicit PpInstance(PP_Instance instance)
      : pp::Instance(instance),
        request_handling_mutex_(std::make_shared<std::mutex>()),
        pcsc_lite_over_requester_global_(new gsc::PcscLiteOverRequesterGlobal(
            &typed_message_router_, this, pp::Module::Get()->core())),
        built_in_pin_dialog_server_(new BuiltInPinDialogServer(
            &typed_message_router_, this, pp::Module::Get()->core())),
        chrome_certificate_provider_api_bridge_(new ccp::ApiBridge(
            &typed_message_router_, this, pp::Module::Get()->core(),
            request_handling_mutex_)),
        ui_bridge_(new UiBridge(&typed_message_router_, this,
                                request_handling_mutex_)),
        certificates_request_handler_(new ClientCertificatesRequestHandler),
        signature_request_handler_(new ClientSignatureRequestHandler(
            chrome_certificate_provider_api_bridge_)),
        message_from_ui_handler_(new ClientMessageFromUiHandler(
            ui_bridge_, built_in_pin_dialog_server_)) {
    chrome_certificate_provider_api_bridge_->SetCertificatesRequestHandler(
        certificates_request_handler_);
    chrome_certificate_provider_api_bridge_->SetSignatureRequestHandler(
        signature_request_handler_);
    ui_bridge_->SetHandler(message_from_ui_handler_);
    StartWorkInBackgroundThread();
  }

  // The destructor that is executed when the NaCl framework is about to destroy
  // the NaCl module (though, actually, it's not guaranteed to be executed at
  // all - the NaCl module can be just shut down by the browser).
  //
  // The implementation presented here essentially leaves the previously
  // allocated google_smart_card::PcscLiteOverRequesterGlobal not destroyed
  // (i.e. leaves it as a leaked pointer). This is done intentionally: there may
  // be still PC/SC-Lite API function calls being executed, and they are using
  // the common state provided by this
  // google_smart_card::PcscLiteOverRequesterGlobal object. So, instead of
  // deleting it (which may lead to undefined behavior), the Detach method of
  // this class is called - which prevents it from using pointer to this
  // instance of PpInstance class.
  ~PpInstance() override {
    pcsc_lite_over_requester_global_->Detach();
    pcsc_lite_over_requester_global_.release();

    built_in_pin_dialog_server_->Detach();
    chrome_certificate_provider_api_bridge_->Detach();
    ui_bridge_->Detach();
  }

  // This method is called with each message received by the NaCl module from
  // the JavaScript side.
  //
  // All the messages are processed through the
  // google_smart_card::TypedMessageRouter class instance, which routes them to
  // the objects that subscribed for receiving them. The routing is based on the
  // "type" key of the message (for the description of the typed messages, see
  // header common/cpp/src/google_smart_card_common/messaging/typed_message.h).
  //
  // In the implementation presented here, the following messages are received
  // and handled here:
  // * results of the submitted PC/SC-Lite API calls (see the
  //   google_smart_card::PcscLiteOverRequesterGlobal class);
  // * requests and responses sent to/received from chrome.certificateProvider
  //   API (see the chrome_certificate_provider::ApiBridge class).
  //
  // Note that this method should not perform any long operations or
  // blocking operations that wait for responses received from the JavaScript
  // side - because this method is called by NaCl framework on the main thread,
  // and blocking it prevents the NaCl module from receiving new incoming
  // messages (see
  // <https://developer.chrome.com/native-client/devguide/coding/message-system>).
  void HandleMessage(const pp::Var& message) override {
    std::string error_message;
    gsc::optional<gsc::Value> message_value =
        gsc::ConvertPpVarToValue(message, &error_message);
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
  // This class is the implementation of the
  // onCertificatesUpdateRequested/onCertificatesRequested requests from the
  // chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesUpdateRequested>
  // and
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>).
  class ClientCertificatesRequestHandler final
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
    bool HandleRequest(
        std::vector<ccp::ClientCertificateInfo>* result) override {
      GetCertificates(result);
      return true;
    }
  };

  // This class is the implementation of the
  // onSignatureRequested/onSignDigestRequested requests from the
  // chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignatureRequested>
  // and
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>).
  class ClientSignatureRequestHandler final
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

  class ClientMessageFromUiHandler final : public MessageFromUiHandler {
   public:
    explicit ClientMessageFromUiHandler(
        std::weak_ptr<UiBridge> ui_bridge,
        std::weak_ptr<BuiltInPinDialogServer> built_in_pin_dialog_server)
        : ui_bridge_(ui_bridge),
          built_in_pin_dialog_server_(built_in_pin_dialog_server) {}

    void HandleMessageFromUi(const pp::Var& message) override {
      //
      // CHANGE HERE:
      // Place your custom code here:
      //

      pp::VarDictionary message_dict;
      std::string command;
      std::string error_message;
      if (gsc::VarAs(message, &message_dict, &error_message) &&
          gsc::GetVarDictValueAs(message_dict, /*key=*/"command", &command,
                                 &error_message) &&
          command == "run_test") {
        OnRunTestCommandReceived();
        return;
      }
      GOOGLE_SMART_CARD_LOG_ERROR << "Unexpected message from UI: "
                                  << gsc::DebugDumpVar(message);
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
      if (!locked_ui_bridge) return;
      locked_ui_bridge->SendMessageToUi(
          gsc::VarDictBuilder().Add("output_message", text).Result());
    }

    std::weak_ptr<UiBridge> ui_bridge_;
    const std::weak_ptr<BuiltInPinDialogServer> built_in_pin_dialog_server_;
  };

  // This method is called by the constructor once all of the initialization
  // steps finish.
  void StartWorkInBackgroundThread() {
    std::thread(&PpInstance::Work, chrome_certificate_provider_api_bridge_)
        .detach();
  }

  // This method is executed on a background thread after all of the
  // initialization steps finish.
  static void Work(
      std::weak_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge) {
    //
    // CHANGE HERE:
    // Place your custom initialization code here:
    //

    // Report the currently available list of certificates after the
    // initialization is done and all available certificates are known,
    // as per the requirements imposed by Chrome - see
    // <https://developer.chrome.com/extensions/certificateProvider#method-setCertificates>.
    ReportAvailableCertificates(chrome_certificate_provider_api_bridge);
  }

  // Mutex that enforces that all requests are handled sequentially.
  const std::shared_ptr<std::mutex> request_handling_mutex_;
  // Router of the incoming typed messages that passes incoming messages to the
  // appropriate handlers according the the special type field of the message
  // (see common/cpp/src/google_smart_card_common/messaging/typed_message.h).
  gsc::TypedMessageRouter typed_message_router_;
  // Object that initializes the global common state used by the PC/SC-Lite
  // client API functions.
  //
  // The stored pointer is leaked intentionally in the class destructor - see
  // its comment for the justification.
  std::unique_ptr<gsc::PcscLiteOverRequesterGlobal>
      pcsc_lite_over_requester_global_;
  // Object that allows to perform built-in PIN dialog requests.
  std::shared_ptr<BuiltInPinDialogServer> built_in_pin_dialog_server_;
  // Object that allows to make calls to and receive events from the
  // chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#events>).
  std::shared_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge_;
  // Object that sends/receives messages to/from the UI.
  const std::shared_ptr<UiBridge> ui_bridge_;
  // Handler of the onCertificateUpdatesRequested/onCertificatesRequested
  // requests from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesUpdateRequested>
  // and
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>).
  const std::shared_ptr<ClientCertificatesRequestHandler>
      certificates_request_handler_;
  // Handler of the onSignatureRequested/onSignDigestRequested requests
  // from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignatureRequested>
  // and
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>).
  const std::shared_ptr<ClientSignatureRequestHandler>
      signature_request_handler_;
  // Handler of messages from UI.
  const std::shared_ptr<ClientMessageFromUiHandler> message_from_ui_handler_;
};

// This class represents the NaCl module for the NaCl framework.
//
// Note that potentially the NaCl framework can request creating multiple
// pp::Instance objects through this module object, though, actually, this never
// happens with the current NaCl framework (and with no exact plans to change it
// in the future: see <http://crbug.com/385783>).
class PpModule final : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) override {
    return new PpInstance(instance);
  }
};

}  // namespace

}  // namespace smart_card_client

namespace pp {

// Entry point of the NaCl module, that is called by the NaCl framework when the
// module is being loaded.
Module* CreateModule() { return new scc::PpModule; }

}  // namespace pp
