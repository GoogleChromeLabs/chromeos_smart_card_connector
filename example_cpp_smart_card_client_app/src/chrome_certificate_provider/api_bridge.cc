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

#include "chrome_certificate_provider/api_bridge.h"

#include <string>
#include <thread>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/unique_ptr_utils.h>

const char kRequestReceiverName[] = "certificate_provider_bridge";
const char kHandleCertificatesRequestFunctionName[] =
    "HandleCertificatesRequest";
const char kHandleSignDigestRequestFunctionName[] =
    "HandleSignDigestRequest";

namespace scc = smart_card_client;
namespace ccp = scc::chrome_certificate_provider;
namespace gsc = google_smart_card;

namespace smart_card_client {

namespace chrome_certificate_provider {

namespace {

void ProcessCertificatesRequest(
    std::weak_ptr<CertificatesRequestHandler> certificates_request_handler,
    gsc::RequestReceiver::ResultCallback result_callback) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "Processing certificates request...";
  std::vector<CertificateInfo> certificates;
  const std::shared_ptr<CertificatesRequestHandler>
  locked_certificates_request_handler = certificates_request_handler.lock();
  GOOGLE_SMART_CARD_CHECK(locked_certificates_request_handler);
  if (locked_certificates_request_handler->HandleRequest(&certificates)) {
    result_callback(gsc::GenericRequestResult::CreateSuccessful(
        gsc::MakeVarArray(certificates)));
  } else {
    result_callback(gsc::GenericRequestResult::CreateFailed("Failure"));
  }
}

void ProcessSignDigestRequest(
    std::weak_ptr<SignDigestRequestHandler> sign_digest_request_handler,
    const SignRequest& sign_request,
    gsc::RequestReceiver::ResultCallback result_callback) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "Processing sign digest request...";
  std::vector<uint8_t> signature;
  const std::shared_ptr<SignDigestRequestHandler>
  locked_sign_digest_request_handler = sign_digest_request_handler.lock();
  GOOGLE_SMART_CARD_CHECK(locked_sign_digest_request_handler);
  if (locked_sign_digest_request_handler->HandleRequest(
          sign_request, &signature)) {
    result_callback(gsc::GenericRequestResult::CreateSuccessful(
        gsc::MakeVarArray(signature)));
  } else {
    result_callback(gsc::GenericRequestResult::CreateFailed("Failure"));
  }
}

}  // namespace

ApiBridge::ApiBridge(
    gsc::TypedMessageRouter* typed_message_router, pp::Instance* pp_instance)
    : request_receiver_(new gsc::JsRequestReceiver(
          kRequestReceiverName,
          this,
          typed_message_router,
          gsc::MakeUnique<gsc::JsRequestReceiver::PpDelegateImpl>(
              pp_instance))) {}

void ApiBridge::Detach() {
  request_receiver_->Detach();
}

void ApiBridge::SetCertificatesRequestHandler(
    std::weak_ptr<CertificatesRequestHandler> handler) {
  certificates_request_handler_ = handler;
}

void ApiBridge::RemoveCertificatesRequestHandler() {
  certificates_request_handler_.reset();
}

void ApiBridge::SetSignDigestRequestHandler(
    std::weak_ptr<SignDigestRequestHandler> handler) {
  sign_digest_request_handler_ = handler;
}

void ApiBridge::RemoveSignDigestRequestHandler() {
  sign_digest_request_handler_.reset();
}

void ApiBridge::HandleRequest(
    const pp::Var& payload,
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::string function_name;
  pp::VarArray arguments;
  GOOGLE_SMART_CARD_CHECK(gsc::ParseRemoteCallRequestPayload(
      payload, &function_name, &arguments));
  if (function_name == kHandleCertificatesRequestFunctionName) {
    HandleCertificatesRequest(arguments, result_callback);
  } else if (function_name == kHandleSignDigestRequestFunctionName) {
    HandleSignDigestRequest(arguments, result_callback);
  } else {
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown chrome_certificate_provider " <<
        "ApiBridge function requested: \"" << function_name << "\"";
  }
}

void ApiBridge::HandleCertificatesRequest(
    const pp::VarArray& arguments,
    gsc::RequestReceiver::ResultCallback result_callback) {
  gsc::GetVarArrayItems(arguments);
  std::thread(
      &ProcessCertificatesRequest,
      certificates_request_handler_,
      result_callback).detach();
}

void ApiBridge::HandleSignDigestRequest(
    const pp::VarArray& arguments,
    gsc::RequestReceiver::ResultCallback result_callback) {
  SignRequest sign_request;
  gsc::GetVarArrayItems(arguments, &sign_request);
  std::thread(
      &ProcessSignDigestRequest,
      sign_digest_request_handler_,
      sign_request,
      result_callback).detach();
}

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client
