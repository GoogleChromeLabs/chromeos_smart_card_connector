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

#include <memory>
#include <thread>
#include <utility>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/function_call_tracer.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/requesting/remote_call_arguments_conversion.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"

namespace scc = smart_card_client;
namespace gsc = google_smart_card;

namespace smart_card_client {

namespace chrome_certificate_provider {

namespace {

// These constants must match the ones in bridge-backend.js.
constexpr char kRequesterName[] =
    "certificate_provider_request_from_executable";
constexpr char kRequestReceiverName[] =
    "certificate_provider_request_to_executable";
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
    gsc::Value response(gsc::Value::Type::kArray);
    response.GetArray().push_back(std::make_unique<gsc::Value>(
        gsc::ConvertToValueOrDie(std::move(certificates))));
    result_callback(
        gsc::GenericRequestResult::CreateSuccessful(std::move(response)));
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
    gsc::Value response(gsc::Value::Type::kArray);
    response.GetArray().push_back(std::make_unique<gsc::Value>(
        gsc::ConvertToValueOrDie(std::move(signature))));
    result_callback(
        gsc::GenericRequestResult::CreateSuccessful(std::move(response)));
  } else {
    result_callback(gsc::GenericRequestResult::CreateFailed("Failure"));
  }
}

}  // namespace

ApiBridge::ApiBridge(gsc::GlobalContext* global_context,
                     gsc::TypedMessageRouter* typed_message_router,
                     std::shared_ptr<std::mutex> request_handling_mutex)
    : request_handling_mutex_(request_handling_mutex),
      requester_(kRequesterName, global_context, typed_message_router),
      remote_call_adaptor_(&requester_),
      request_receiver_(new gsc::JsRequestReceiver(kRequestReceiverName,
                                                   this,
                                                   global_context,
                                                   typed_message_router)) {}

ApiBridge::~ApiBridge() = default;

void ApiBridge::ShutDown() {
  requester_.ShutDown();
  request_receiver_->ShutDown();
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
  gsc::Value options_value = gsc::ConvertToValueOrDie(options);

  gsc::FunctionCallTracer tracer("requestPin", kFunctionCallLoggingPrefix,
                                 gsc::LogSeverity::kInfo);
  tracer.AddPassedArg("options", gsc::DebugDumpValueFull(options_value));
  tracer.LogEntrance();

  gsc::GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("requestPin", std::move(options_value));
  if (!generic_request_result.is_successful()) {
    tracer.AddReturnValue(
        "false (error: " + generic_request_result.error_message() + ")");
    tracer.LogExit();
    return false;
  }
  RequestPinResults results;
  gsc::RemoteCallArgumentsExtractor extractor(
      "result of requestPin", std::move(generic_request_result).TakePayload());
  // The Chrome API can omit the result object.
  if (extractor.argument_count() > 0)
    extractor.Extract(&results);
  if (!extractor.Finish())
    GOOGLE_SMART_CARD_LOG_FATAL << extractor.error_message();
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
  gsc::Value options_value = gsc::ConvertToValueOrDie(options);

  gsc::FunctionCallTracer tracer("stopPinRequest", kFunctionCallLoggingPrefix,
                                 gsc::LogSeverity::kInfo);
  tracer.AddPassedArg("options", gsc::DebugDumpValueFull(options_value));
  tracer.LogEntrance();

  const gsc::GenericRequestResult generic_request_result =
      remote_call_adaptor_.SyncCall("stopPinRequest", std::move(options_value));
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
    gsc::Value payload,
    gsc::RequestReceiver::ResultCallback result_callback) {
  gsc::RemoteCallRequestPayload request =
      gsc::ConvertFromValueOrDie<gsc::RemoteCallRequestPayload>(
          std::move(payload));
  if (request.function_name == kHandleCertificatesRequestFunctionName) {
    gsc::ExtractRemoteCallArgumentsOrDie(std::move(request.function_name),
                                         std::move(request.arguments));
    HandleCertificatesRequest(result_callback);
  } else if (request.function_name == kHandleSignatureRequestFunctionName) {
    SignatureRequest signature_request;
    gsc::ExtractRemoteCallArgumentsOrDie(std::move(request.function_name),
                                         std::move(request.arguments),
                                         &signature_request);
    HandleSignatureRequest(std::move(signature_request), result_callback);
  } else {
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown chrome_certificate_provider "
                                << "ApiBridge function requested: \""
                                << request.function_name << "\"";
  }
}

void ApiBridge::HandleCertificatesRequest(
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::thread(&ProcessCertificatesRequest, certificates_request_handler_,
              request_handling_mutex_, result_callback)
      .detach();
}

void ApiBridge::HandleSignatureRequest(
    SignatureRequest signature_request,
    gsc::RequestReceiver::ResultCallback result_callback) {
  std::thread(&ProcessSignatureRequest, signature_request_handler_,
              std::move(signature_request), request_handling_mutex_,
              result_callback)
      .detach();
}

}  // namespace chrome_certificate_provider

}  // namespace smart_card_client
