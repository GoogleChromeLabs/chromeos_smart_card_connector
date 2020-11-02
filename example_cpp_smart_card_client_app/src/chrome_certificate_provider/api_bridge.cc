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

#include <thread>
#include <utility>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/logging/function_call_tracer.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

namespace scc = smart_card_client;
namespace ccp = scc::chrome_certificate_provider;
namespace gsc = google_smart_card;

namespace smart_card_client {

namespace chrome_certificate_provider {

namespace {

constexpr char kRequesterName[] = "certificate_provider_nacl_outgoing";
constexpr char kRequestReceiverName[] = "certificate_provider_nacl_incoming";
constexpr char kHandleCertificatesRequestFunctionName[] =
    "HandleCertificatesRequest";
constexpr char kHandleSignatureRequestFunctionName[] = "HandleSignatureRequest";
constexpr char kFunctionCallLoggingPrefix[] = "chrome.certificateProvider.";

void ProcessCertificatesRequest(
    std::weak_ptr<CertificatesRequestHandler> certificates_request_handler,
    std::shared_ptr<std::mutex> request_handling_mutex,
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::unique_lock<std::mutex> lock;
  if (request_handling_mutex)
    lock = std::unique_lock<std::mutex>(*request_handling_mutex);

  GOOGLE_SMART_CARD_LOG_DEBUG << "Processing certificates request...";
  std::vector<ClientCertificateInfo> certificates;
  const std::shared_ptr<CertificatesRequestHandler>
      locked_certificates_request_handler = certificates_request_handler.lock();
  GOOGLE_SMART_CARD_CHECK(locked_certificates_request_handler);
  if (locked_certificates_request_handler->HandleRequest(&certificates)) {
    // TODO(#220): Build `Value` directly, without converting from `pp::Var`.
    result_callback(gsc::GenericRequestResult::CreateSuccessful(
        gsc::ConvertPpVarToValueOrDie(gsc::MakeVarArray(certificates))));
  } else {
    result_callback(gsc::GenericRequestResult::CreateFailed("Failure"));
  }
}

void ProcessSignatureRequest(
    std::weak_ptr<SignatureRequestHandler> signature_request_handler,
    const SignatureRequest& signature_request,
    std::shared_ptr<std::mutex> request_handling_mutex,
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::unique_lock<std::mutex> lock;
  if (request_handling_mutex)
    lock = std::unique_lock<std::mutex>(*request_handling_mutex);

  GOOGLE_SMART_CARD_LOG_DEBUG << "Processing sign digest request...";
  std::vector<uint8_t> signature;
  const std::shared_ptr<SignatureRequestHandler>
      locked_signature_request_handler = signature_request_handler.lock();
  GOOGLE_SMART_CARD_CHECK(locked_signature_request_handler);
  if (locked_signature_request_handler->HandleRequest(signature_request,
                                                      &signature)) {
    // TODO(#220): Build `Value` directly, without converting from `pp::Var`.
    result_callback(gsc::GenericRequestResult::CreateSuccessful(
        gsc::ConvertPpVarToValueOrDie(gsc::MakeVarArrayBuffer(signature))));
  } else {
    result_callback(gsc::GenericRequestResult::CreateFailed("Failure"));
  }
}

}  // namespace

ApiBridge::ApiBridge(gsc::TypedMessageRouter* typed_message_router,
                     pp::Instance* pp_instance, pp::Core* pp_core,
                     std::shared_ptr<std::mutex> request_handling_mutex)
    : request_handling_mutex_(request_handling_mutex),
      requester_(kRequesterName, typed_message_router,
                 gsc::MakeUnique<gsc::JsRequester::PpDelegateImpl>(pp_instance,
                                                                   pp_core)),
      remote_call_adaptor_(&requester_),
      request_receiver_(new gsc::JsRequestReceiver(
          kRequestReceiverName, this, typed_message_router,
          gsc::MakeUnique<gsc::JsRequestReceiver::PpDelegateImpl>(
              pp_instance))) {}

ApiBridge::~ApiBridge() = default;

void ApiBridge::Detach() {
  requester_.Detach();
  request_receiver_->Detach();
}

void ApiBridge::SetCertificatesRequestHandler(
    std::weak_ptr<CertificatesRequestHandler> handler) {
  certificates_request_handler_ = handler;
}

void ApiBridge::RemoveCertificatesRequestHandler() {
  certificates_request_handler_.reset();
}

void ApiBridge::SetSignatureRequestHandler(
    std::weak_ptr<SignatureRequestHandler> handler) {
  signature_request_handler_ = handler;
}

void ApiBridge::RemoveSignatureRequestHandler() {
  signature_request_handler_.reset();
}

void ApiBridge::SetCertificates(
    const std::vector<ClientCertificateInfo>& certificates) {
  SetCertificatesDetails details;
  details.client_certificates = certificates;
  remote_call_adaptor_.SyncCall("setCertificates", details);
}

bool ApiBridge::RequestPin(const RequestPinOptions& options, std::string* pin) {
  gsc::FunctionCallTracer tracer("requestPin", kFunctionCallLoggingPrefix,
                                 gsc::LogSeverity::kInfo);
  tracer.AddPassedArg("options", gsc::DumpVar(MakeVar(options)));
  tracer.LogEntrance();

  const gsc::GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("requestPin", options);
  if (!generic_request_result.is_successful()) {
    tracer.AddReturnValue(
        "false (error: " + generic_request_result.error_message() + ")");
    tracer.LogExit();
    return false;
  }
  // TODO(#220): Parse `Value` directly, rather than convert into `pp::Var`.
  const pp::Var payload_var =
      gsc::ConvertValueToPpVar(generic_request_result.payload());
  // Note: Cannot use RemoteCallAdaptor::ExtractResultPayload(), since the API
  // result value is defined as optional, so that the length of the array we
  // parse here isn't fixed.
  pp::VarArray var_array;
  std::string error_message;
  if (!gsc::VarAs(payload_var, &var_array, &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Failed to extract the PIN response "
                                << "payload items: " << error_message;
  }
  RequestPinResults results;
  if (gsc::GetVarArraySize(var_array) &&
      !gsc::TryGetVarArrayItems(var_array, &error_message, &results)) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Failed to extract the PIN response "
                                << "payload items: " << error_message;
  }
  if (!results.user_input || results.user_input->empty()) {
    tracer.AddReturnValue("false (empty PIN)");
    tracer.LogExit();
    return false;
  }
  *pin = std::move(*results.user_input);
  tracer.AddReturnValue("true (success)");
  tracer.LogExit();
  return true;
}

void ApiBridge::StopPinRequest(const StopPinRequestOptions& options) {
  gsc::FunctionCallTracer tracer("stopPinRequest", kFunctionCallLoggingPrefix,
                                 gsc::LogSeverity::kInfo);
  tracer.AddPassedArg("options", gsc::DumpVar(MakeVar(options)));
  tracer.LogEntrance();

  const gsc::GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("stopPinRequest", options);
  if (!generic_request_result.is_successful()) {
    tracer.AddReturnValue("error (" + generic_request_result.error_message() +
                          ")");
    tracer.LogExit();
    return;
  }
  tracer.AddReturnValue("success");
  tracer.LogExit();
}

void ApiBridge::HandleRequest(
    const pp::Var& payload,
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::string function_name;
  pp::VarArray arguments;
  GOOGLE_SMART_CARD_CHECK(
      gsc::ParseRemoteCallRequestPayload(payload, &function_name, &arguments));
  if (function_name == kHandleCertificatesRequestFunctionName) {
    HandleCertificatesRequest(arguments, result_callback);
  } else if (function_name == kHandleSignatureRequestFunctionName) {
    HandleSignatureRequest(arguments, result_callback);
  } else {
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown chrome_certificate_provider "
                                << "ApiBridge function requested: \""
                                << function_name << "\"";
  }
}

void ApiBridge::HandleCertificatesRequest(
    const pp::VarArray& arguments,
    gsc::RequestReceiver::ResultCallback result_callback) {
  gsc::GetVarArrayItems(arguments);
  std::thread(&ProcessCertificatesRequest, certificates_request_handler_,
              request_handling_mutex_, result_callback)
      .detach();
}

void ApiBridge::HandleSignatureRequest(
    const pp::VarArray& arguments,
    gsc::RequestReceiver::ResultCallback result_callback) {
  SignatureRequest signature_request;
  gsc::GetVarArrayItems(arguments, &signature_request);
  std::thread(&ProcessSignatureRequest, signature_request_handler_,
              signature_request, request_handling_mutex_, result_callback)
      .detach();
}

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client
