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

// This file provides definitions that provide the C++ bridge to the
// chrome.certificateProvider JavaScript API (see
// <https://developer.chrome.com/extensions/certificateProvider>).

#ifndef SMART_CARD_CLIENT_CHROME_CERTIFICATE_PROVIDER_API_BRIDGE_H_
#define SMART_CARD_CLIENT_CHROME_CERTIFICATE_PROVIDER_API_BRIDGE_H_

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/js_request_receiver.h>
#include <google_smart_card_common/requesting/js_requester.h>
#include <google_smart_card_common/requesting/remote_call_adaptor.h>
#include <google_smart_card_common/requesting/request_handler.h>
#include <google_smart_card_common/requesting/request_receiver.h>
#include <google_smart_card_common/value.h>

#include "chrome_certificate_provider/types.h"

namespace smart_card_client {

namespace chrome_certificate_provider {

// Handler of the certificates listing request.
//
// For the related original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesUpdateRequested>
// and
// <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>.
class CertificatesRequestHandler {
 public:
  virtual ~CertificatesRequestHandler() = default;

  virtual bool HandleRequest(std::vector<ClientCertificateInfo>* result) = 0;
};

// Handler of the signature request.
//
// For the related original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#event-onSignatureRequested>
// and
// <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>.
class SignatureRequestHandler {
 public:
  virtual ~SignatureRequestHandler() = default;

  virtual bool HandleRequest(const SignatureRequest& signature_request,
                             std::vector<uint8_t>* result) = 0;
};

// This class provides a C++ bridge to the chrome.certificateProvider JavaScript
// API (see <https://developer.chrome.com/extensions/certificateProvider>).
//
// The bridge is bidirectional: it allows both to make requests to Chrome and to
// receive events sent by Chrome.
//
// Under the hood, this class is implemented by sending and receiving messages
// of special form to/from the corresponding backend on the JavaScript side (the
// bridge-backend.js file), with the latter transforming them to/from the actual
// chrome.certificateProvider method calls and events.
class ApiBridge final : public google_smart_card::RequestHandler {
 public:
  // Creates the bridge instance.
  //
  // On construction, registers self for receiving the corresponding request
  // messages through the supplied TypedMessageRouter typed_message_router
  // instance.
  //
  // |global_context| - must outlive |this|.
  // The |request_handling_mutex| parameter, when non-null, allows to avoid
  // simultaneous execution of multiple requests: each next request will be
  // executed only once the previous one finishes.
  ApiBridge(google_smart_card::GlobalContext* global_context,
            google_smart_card::TypedMessageRouter* typed_message_router,
            std::shared_ptr<std::mutex> request_handling_mutex);

  ApiBridge(const ApiBridge&) = delete;
  ApiBridge& operator=(const ApiBridge&) = delete;

  ~ApiBridge() override;

  void ShutDown();

  void SetCertificatesRequestHandler(
      std::weak_ptr<CertificatesRequestHandler> handler);
  void RemoveCertificatesRequestHandler();

  void SetSignatureRequestHandler(
      std::weak_ptr<SignatureRequestHandler> handler);
  void RemoveSignatureRequestHandler();

  // Sends the current list of certificates in Chrome. The should be called
  // after initialization and on every change in the set of currently available
  // certificates.
  void SetCertificates(const std::vector<ClientCertificateInfo>& certificates);

  // Sends a PIN request and waits for the response being received.
  //
  // Returns whether the PIN dialog finished successfully, and, if yes, returns
  // the PIN entered by user through the pin output argument.
  //
  // Note that this function must not be called from the main thread, because
  // otherwise it would block receiving of the incoming messages and,
  // consequently, it would lock forever. (Actually, the validity of the current
  // thread is asserted inside.)
  bool RequestPin(const RequestPinOptions& options, std::string* pin);

  // Stops the PIN request that was previously started by the RequestPin()
  // function.
  //
  // Note that this function must not be called from the main thread, because
  // otherwise it would block receiving of the incoming messages and,
  // consequently, it would lock forever. (Actually, the validity of the current
  // thread is asserted inside.)
  void StopPinRequest(const StopPinRequestOptions& options);

 private:
  // google_smart_card::RequestHandler:
  void HandleRequest(google_smart_card::Value payload,
                     google_smart_card::RequestReceiver::ResultCallback
                         result_callback) override;

  void HandleCertificatesRequest(
      google_smart_card::RequestReceiver::ResultCallback result_callback);
  void HandleSignatureRequest(
      SignatureRequest signature_request,
      google_smart_card::RequestReceiver::ResultCallback result_callback);

  std::shared_ptr<std::mutex> request_handling_mutex_;

  // Members related to outgoing requests:

  google_smart_card::JsRequester requester_;
  google_smart_card::RemoteCallAdaptor remote_call_adaptor_;

  // Members related to incoming requests:

  std::shared_ptr<google_smart_card::JsRequestReceiver> request_receiver_;
  std::weak_ptr<CertificatesRequestHandler> certificates_request_handler_;
  std::weak_ptr<SignatureRequestHandler> signature_request_handler_;
};

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client

#endif  // SMART_CARD_CLIENT_CHROME_CERTIFICATE_PROVIDER_API_BRIDGE_H_
