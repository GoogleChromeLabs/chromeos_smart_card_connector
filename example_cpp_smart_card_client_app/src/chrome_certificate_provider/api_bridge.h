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
#include <vector>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/js_request_receiver.h>
#include <google_smart_card_common/requesting/request_handler.h>
#include <google_smart_card_common/requesting/request_receiver.h>

#include "chrome_certificate_provider/types.h"

namespace smart_card_client {

namespace chrome_certificate_provider {

// Handler of the certificates request.
//
// For the related original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>
class CertificatesRequestHandler {
 public:
  virtual ~CertificatesRequestHandler() = default;

  virtual bool HandleRequest(std::vector<CertificateInfo>* result) = 0;
};

// Handler of the digest signing request.
//
// For the related original JavaScript definition, refer to:
// <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>
class SignDigestRequestHandler {
 public:
  virtual ~SignDigestRequestHandler() = default;

  virtual bool HandleRequest(
      const SignRequest& sign_request, std::vector<uint8_t>* result) = 0;
};

// This class provides a C++ bridge to the chrome.certificateProvider JavaScript
// API (see <https://developer.chrome.com/extensions/certificateProvider>).
//
// The integration with the JavaScript API is done by performing the requests of
// some special form to the JavaScript side. On the JavaScript side, the handler
// of these requests will call the corresponding chrome.usb API methods (see the
// bridge-backend.js files).
class ApiBridge final : public google_smart_card::RequestHandler {
 public:
  ApiBridge(
      google_smart_card::TypedMessageRouter* typed_message_router,
      pp::Instance* pp_instance);

  void Detach();

  void SetCertificatesRequestHandler(
      std::weak_ptr<CertificatesRequestHandler> handler);
  void RemoveCertificatesRequestHandler();

  void SetSignDigestRequestHandler(
      std::weak_ptr<SignDigestRequestHandler> handler);
  void RemoveSignDigestRequestHandler();

 private:
  void HandleRequest(
      const pp::Var& payload,
      google_smart_card::RequestReceiver::ResultCallback result_callback)
  override;

  void HandleCertificatesRequest(
      const pp::VarArray& arguments,
      google_smart_card::RequestReceiver::ResultCallback result_callback);
  void HandleSignDigestRequest(
      const pp::VarArray& arguments,
      google_smart_card::RequestReceiver::ResultCallback result_callback);

  std::shared_ptr<google_smart_card::JsRequestReceiver> request_receiver_;
  std::weak_ptr<CertificatesRequestHandler> certificates_request_handler_;
  std::weak_ptr<SignDigestRequestHandler> sign_digest_request_handler_;
};

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client

#endif  // SMART_CARD_CLIENT_CHROME_CERTIFICATE_PROVIDER_API_BRIDGE_H_
