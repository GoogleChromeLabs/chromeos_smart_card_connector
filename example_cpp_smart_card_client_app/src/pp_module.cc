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
#include <string>
#include <thread>
#include <vector>

#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_pcsc_lite_client/global.h>
#include <google_smart_card_pcsc_lite_cpp_demo/demo.h>

#include "chrome_certificate_provider/api_bridge.h"
#include "chrome_certificate_provider/types.h"

namespace scc = smart_card_client;
namespace ccp = scc::chrome_certificate_provider;
namespace gsc = google_smart_card;

namespace smart_card_client {

namespace {

// This class contains the actual NaCl module implementation.
//
// The implementation presented here is a skeleton that initializes all pieces
// necessary for PC/SC-Lite client API initialization,
// chrome.certificateProvider JavaScript API integration.
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
  //   <https://developer.chrome.com/extensions/certificateProvider#events>).
  explicit PpInstance(PP_Instance instance)
      : pp::Instance(instance),
        pcsc_lite_over_requester_global_(
            new gsc::PcscLiteOverRequesterGlobal(
                &typed_message_router_, this, pp::Module::Get()->core())),
        chrome_certificate_provider_api_bridge_(
            new ccp::ApiBridge(
                &typed_message_router_,
                this,
                pp::Module::Get()->core(),
                /* execute_requests_sequentially */ false)),
        certificates_request_handler_(new ClientCertificatesRequestHandler),
        sign_digest_request_handler_(new ClientSignDigestRequestHandler(
            chrome_certificate_provider_api_bridge_)) {
    chrome_certificate_provider_api_bridge_->SetCertificatesRequestHandler(
        certificates_request_handler_);
    chrome_certificate_provider_api_bridge_->SetSignDigestRequestHandler(
        sign_digest_request_handler_);
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

    chrome_certificate_provider_api_bridge_->Detach();
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
    if (!typed_message_router_.OnMessageReceived(message)) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected message received: " <<
          gsc::DebugDumpVar(message);
    }
  }

 private:
  // This class is the implementation of the onCertificatesRequested request
  // from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>).
  class ClientCertificatesRequestHandler final
      : public ccp::CertificatesRequestHandler {
   public:
    // Handles the received certificates request.
    //
    // Returns whether the operation finished successfully. In case of success,
    // the resulting certificates information should be returned through the
    // result output argument.
    //
    // Note that this method is executed by
    // chrome_certificate_provider::ApiBridge object on a separate background
    // thread. Multiple requests can be executed simultaneously (they will run
    // in different background threads).
    bool HandleRequest(std::vector<ccp::CertificateInfo>* result) override {
      //
      // CHANGE HERE:
      // Place your custom code here:
      //

      ccp::CertificateInfo certificate_info_1;
      certificate_info_1.certificate.assign({1, 2, 3});
      certificate_info_1.supported_hashes.push_back(ccp::Hash::kMd5Sha1);
      ccp::CertificateInfo certificate_info_2;
      certificate_info_2.supported_hashes.push_back(ccp::Hash::kSha512);
      result->push_back(certificate_info_1);
      result->push_back(certificate_info_2);
      return true;
    }
  };

  // This class is the implementation of the onSignDigestRequested request
  // from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>).
  class ClientSignDigestRequestHandler final
      : public ccp::SignDigestRequestHandler {
   public:
    explicit ClientSignDigestRequestHandler(
        std::weak_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge)
        : chrome_certificate_provider_api_bridge_(
              chrome_certificate_provider_api_bridge) {}

    // Handles the received sign digest request (the request data is passed
    // through the sign_request argument).
    //
    // Returns whether the operation finished successfully. In case of success,
    // the resulting signature should be returned through the result output
    // argument.
    //
    // Note that this method is executed by
    // chrome_certificate_provider::ApiBridge object on a separate background
    // thread. Multiple requests can be executed simultaneously (they will run
    // in different background threads).
    bool HandleRequest(
        const ccp::SignRequest& sign_request,
        std::vector<uint8_t>* result) override {
      //
      // CHANGE HERE:
      // Place your custom code here:
      //

      *result = std::vector<uint8_t>({4, 5, 6});

      const std::shared_ptr<ccp::ApiBridge>
          locked_chrome_certificate_provider_api_bridge =
              chrome_certificate_provider_api_bridge_.lock();
      if (!locked_chrome_certificate_provider_api_bridge) {
        GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] Skipped PIN dialog " <<
            "demo: the shutdown process has started";
        return false;
      }

      GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] Running PIN dialog " <<
          "demo...";
      ccp::RequestPinOptions request_pin_options;
      request_pin_options.sign_request_id = sign_request.sign_request_id;
      std::string pin;
      if (!locked_chrome_certificate_provider_api_bridge->RequestPin(
               request_pin_options, &pin)) {
        GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] demo finished: " <<
            "dialog was canceled.";
        return false;
      }

      ccp::StopPinRequestOptions stop_pin_request_options;
      stop_pin_request_options.sign_request_id = sign_request.sign_request_id;
      locked_chrome_certificate_provider_api_bridge->StopPinRequest(
          stop_pin_request_options);

      GOOGLE_SMART_CARD_LOG_INFO << "[PIN Dialog DEMO] demo finished: " <<
          "received PIN of length " << pin.length() << " entered by the user.";
      return true;
    }

   private:
    const std::weak_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge_;
  };

  // This method is called by the constructor once all of the initialization
  // steps finish.
  void StartWorkInBackgroundThread() {
    std::thread(&PpInstance::Work).detach();
  }

  // This method is executed on a background thread after all of the
  // initialization steps finish.
  static void Work() {
    //
    // CHANGE HERE:
    // Place your custom code here:
    //

    GOOGLE_SMART_CARD_LOG_INFO << "[PC/SC-Lite DEMO] Starting PC/SC-Lite " <<
        "demo...";
    if (gsc::ExecutePcscLiteCppDemo()) {
      GOOGLE_SMART_CARD_LOG_INFO << "[PC/SC-Lite DEMO] demo finished " <<
          "successfully.";
    } else {
      GOOGLE_SMART_CARD_LOG_ERROR << "[PC/SC-Lite DEMO] demo failed.";
    }
  }

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
  // Object that allows to receive and handle requests received from the
  // chrome.certificateProvider JavaScript API event listeners (see
  // <https://developer.chrome.com/extensions/certificateProvider#events>).
  std::shared_ptr<ccp::ApiBridge> chrome_certificate_provider_api_bridge_;
  // Handler of the onCertificatesRequested request
  // from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>).
  const std::shared_ptr<ClientCertificatesRequestHandler>
  certificates_request_handler_;
  // Handler of the onSignDigestRequested request
  // from the chrome.certificateProvider JavaScript API (see
  // <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>).
  const std::shared_ptr<ClientSignDigestRequestHandler>
  sign_digest_request_handler_;
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
Module* CreateModule() {
  return new scc::PpModule;
}

}  // namespace pp
