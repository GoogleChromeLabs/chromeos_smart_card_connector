// Copyright 2016 Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "third_party/pcsc-lite/webport/server_clients_management/src/client_request_processor.h"

#include <inttypes.h>
#include <stdlib.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "common/cpp/src/public/formatting.h"
#include "common/cpp/src/public/join_string.h"
#include "common/cpp/src/public/logging/function_call_tracer.h"
#include "common/cpp/src/public/logging/hex_dumping.h"
#include "common/cpp/src/public/multi_string.h"
#include "common/cpp/src/public/requesting/remote_call_arguments_conversion.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/cpp/src/public/value_debug_dumping.h"
#include "third_party/pcsc-lite/webport/common/src/public/scard_debug_dump.h"
#include "third_party/pcsc-lite/webport/server_clients_management/src/admin_policy_getter.h"

namespace google_smart_card {

namespace {

void ConvertToValueVectorOrDie(std::vector<Value>* value_vector) {}

template <typename FirstArg, typename... Args>
void ConvertToValueVectorOrDie(std::vector<Value>* value_vector,
                               const FirstArg& first_arg,
                               const Args&... args) {
  value_vector->push_back(ConvertToValueOrDie(first_arg));
  ConvertToValueVectorOrDie(value_vector, args...);
}

template <typename... Args>
GenericRequestResult ReturnValues(const Args&... args) {
  std::vector<Value> converted_args;
  ConvertToValueVectorOrDie(&converted_args, args...);
  return GenericRequestResult::CreateSuccessful(
      Value(std::move(converted_args)));
}

GenericRequestResult ReturnFailure(const std::string& error_message) {
  return GenericRequestResult::CreateFailed(error_message);
}

// Replacement of the PC/SC-Lite function SCardFreeMemory, that doesn't require
// passing of SCARDCONTEXT, which is not always available in all scopes.
void FreeSCardMemory(void* memory) {
  GOOGLE_SMART_CARD_CHECK(memory);
  ::free(memory);
}

void CancelRunningRequests(const std::string& logging_prefix,
                           LogSeverity error_log_severity,
                           const std::vector<SCARDCONTEXT>& s_card_contexts) {
  for (SCARDCONTEXT s_card_context : s_card_contexts) {
    GOOGLE_SMART_CARD_LOG_DEBUG
        << logging_prefix << "Performing forced "
        << "cleanup: canceling all pending blocking requests for left "
        << "context " << DebugDumpSCardContext(s_card_context);

    const LONG error_code = ::SCardCancel(s_card_context);
    if (error_code != SCARD_S_SUCCESS) {
      GOOGLE_SMART_CARD_LOG(error_log_severity)
          << logging_prefix
          << "Forced cancellation of the blocking requests for context "
          << DebugDumpSCardContext(s_card_context)
          << " was unsuccessful: " << pcsc_stringify_error(error_code);
    }
  }
}

void CloseLeftHandles(const std::string& logging_prefix,
                      const std::vector<SCARDCONTEXT>& s_card_contexts) {
  for (SCARDCONTEXT s_card_context : s_card_contexts) {
    GOOGLE_SMART_CARD_LOG_DEBUG << logging_prefix << "Performing forced "
                                << "cleanup: releasing the left context "
                                << DebugDumpSCardContext(s_card_context);

    const LONG error_code = ::SCardReleaseContext(s_card_context);
    if (error_code == SCARD_S_SUCCESS) {
      GOOGLE_SMART_CARD_LOG_INFO << logging_prefix << "Force released "
                                 << "context "
                                 << DebugDumpSCardContext(s_card_context);
    } else {
      GOOGLE_SMART_CARD_LOG_WARNING
          << logging_prefix << "Forced releasing "
          << "of context " << DebugDumpSCardContext(s_card_context)
          << " was unsuccessful: " << pcsc_stringify_error(error_code);
    }
  }
}

void CleanupHandles(const std::string& logging_prefix,
                    const std::vector<SCARDCONTEXT>& s_card_contexts) {
  CancelRunningRequests(logging_prefix, LogSeverity::kWarning, s_card_contexts);
  CloseLeftHandles(logging_prefix, s_card_contexts);
}

}  // namespace

PcscLiteClientRequestProcessor::PcscLiteClientRequestProcessor(
    int64_t client_handler_id,
    const std::string& client_name_for_log,
    AdminPolicyGetter* admin_policy_getter)
    : client_handler_id_(client_handler_id),
      client_name_for_log_(client_name_for_log),
      admin_policy_getter_(admin_policy_getter),
      status_log_severity_(client_name_for_log_.empty() ? LogSeverity::kDebug
                                                        : LogSeverity::kInfo),
      logging_prefix_(FormatPrintfTemplate("[PC/SC from %s (id %" PRId64 ")] ",
                                           client_name_for_log_.empty()
                                               ? "ourselves"
                                               : client_name_for_log_.c_str(),
                                           client_handler_id)) {
  // Note that `status_log_severity_` is initialized to `kDebug` if it's our own
  // application talking to itself: that way we don't spam with these
  // uninteresting requests in logs in the Release build.
  BuildHandlerMap();
  GOOGLE_SMART_CARD_LOG_DEBUG << logging_prefix_ << "Created client handler";
}

PcscLiteClientRequestProcessor::~PcscLiteClientRequestProcessor() {
  ScheduleHandlesCleanup();
}

void PcscLiteClientRequestProcessor::ScheduleRunningRequestsCancellation() {
  // Obtain the current list of handles associated with this request processor.
  // FIXME(emaxx): There is a small chance of getting data race here, if after
  // this call some background PC/SC-Lite request releases the context, and
  // another background request (in a bad scenario, from a completely different
  // request processor) receives the same context.
  const std::vector<SCARDCONTEXT> s_card_contexts =
      s_card_handles_registry_.GetSnapshotOfAllContexts();

  // The actual cancellation happens in a separate background thread, as the
  // involved SCard* functions may call blocking libusb* functions - which are
  // not allowed to be called from the main thread (attempting to do this will
  // result in a deadlock).
  //
  // Note: the errors inside this function will be logged only at the info
  // level, because this asynchronous call may happen after the context is
  // already released due to the asynchronous job scheduled by the class
  // destructor.
  std::thread(&CancelRunningRequests, logging_prefix_, LogSeverity::kInfo,
              s_card_contexts)
      .detach();
}

void PcscLiteClientRequestProcessor::ProcessRequest(
    RemoteCallRequestPayload request,
    RequestReceiver::ResultCallback result_callback) {
  GOOGLE_SMART_CARD_LOG_DEBUG << logging_prefix_ << "Started processing "
                              << "request " << request.DebugDumpSanitized()
                              << "...";

  GenericRequestResult result =
      FindHandlerAndCall(request.function_name, std::move(request.arguments));

  if (result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_DEBUG
        << logging_prefix_ << "Request " << request.function_name
        << " finished successfully with the following "
        << "results: " << DebugDumpValueSanitized(result.payload());
  } else {
    GOOGLE_SMART_CARD_LOG_DEBUG
        << logging_prefix_ << "Request " << request.function_name
        << " failed with the following error: \"" << result.error_message()
        << "\"";
  }

  result_callback(std::move(result));
}

// static
void PcscLiteClientRequestProcessor::AsyncProcessRequest(
    std::shared_ptr<PcscLiteClientRequestProcessor> request_processor,
    RemoteCallRequestPayload request,
    RequestReceiver::ResultCallback result_callback) {
  std::thread(&PcscLiteClientRequestProcessor::ProcessRequest,
              request_processor, std::move(request), result_callback)
      .detach();
}

PcscLiteClientRequestProcessor::ScopedConcurrencyGuard::ScopedConcurrencyGuard(
    const std::string& function_name,
    SCARDCONTEXT s_card_context,
    PcscLiteClientRequestProcessor& owner)
    : function_name_(function_name),
      s_card_context_(s_card_context),
      owner_(owner) {
  if (!s_card_context_) {
    return;
  }

  optional<std::string> concurrent_functions_dump;
  {
    const std::unique_lock<std::mutex> lock(owner_.mutex_);
    std::multiset<std::string>& running_functions =
        owner_.context_to_running_functions_[s_card_context_];
    if (!running_functions.empty()) {
      concurrent_functions_dump =
          JoinStrings(running_functions, /*separator=*/", ");
    }
    running_functions.insert(function_name_);
  }

  if (concurrent_functions_dump) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << owner_.logging_prefix_
        << "Client violates threading: concurrent calls of " << function_name_
        << ", " << *concurrent_functions_dump
        << " against the same SCARDCONTEXT. Future releases of Smart Card "
           "Connector will forbid this: every call referring to some "
           "SCARDCONTEXT must come after the previous one completed.";
  }
}

PcscLiteClientRequestProcessor::ScopedConcurrencyGuard::
    ~ScopedConcurrencyGuard() {
  if (!s_card_context_) {
    return;
  }
  const std::unique_lock<std::mutex> lock(owner_.mutex_);
  auto iter = owner_.context_to_running_functions_.find(s_card_context_);
  GOOGLE_SMART_CARD_CHECK(iter != owner_.context_to_running_functions_.end());
  std::multiset<std::string>& running_functions = iter->second;
  running_functions.erase(running_functions.find(function_name_));
  if (running_functions.empty()) {
    owner_.context_to_running_functions_.erase(iter);
  }
}

void PcscLiteClientRequestProcessor::BuildHandlerMap() {
  AddHandlerToHandlerMap(
      "pcsc_lite_version_number",
      &PcscLiteClientRequestProcessor::PcscLiteVersionNumber);
  AddHandlerToHandlerMap("pcsc_stringify_error",
                         &PcscLiteClientRequestProcessor::PcscStringifyError);
  AddHandlerToHandlerMap(
      "SCardEstablishContext",
      &PcscLiteClientRequestProcessor::SCardEstablishContext);
  AddHandlerToHandlerMap("SCardReleaseContext",
                         &PcscLiteClientRequestProcessor::SCardReleaseContext);
  AddHandlerToHandlerMap("SCardConnect",
                         &PcscLiteClientRequestProcessor::SCardConnect);
  AddHandlerToHandlerMap("SCardReconnect",
                         &PcscLiteClientRequestProcessor::SCardReconnect);
  AddHandlerToHandlerMap("SCardDisconnect",
                         &PcscLiteClientRequestProcessor::SCardDisconnect);
  AddHandlerToHandlerMap(
      "SCardBeginTransaction",
      &PcscLiteClientRequestProcessor::SCardBeginTransaction);
  AddHandlerToHandlerMap("SCardEndTransaction",
                         &PcscLiteClientRequestProcessor::SCardEndTransaction);
  AddHandlerToHandlerMap("SCardStatus",
                         &PcscLiteClientRequestProcessor::SCardStatus);
  AddHandlerToHandlerMap("SCardGetStatusChange",
                         &PcscLiteClientRequestProcessor::SCardGetStatusChange);
  AddHandlerToHandlerMap("SCardControl",
                         &PcscLiteClientRequestProcessor::SCardControl);
  AddHandlerToHandlerMap("SCardGetAttrib",
                         &PcscLiteClientRequestProcessor::SCardGetAttrib);
  AddHandlerToHandlerMap("SCardSetAttrib",
                         &PcscLiteClientRequestProcessor::SCardSetAttrib);
  AddHandlerToHandlerMap("SCardTransmit",
                         &PcscLiteClientRequestProcessor::SCardTransmit);
  AddHandlerToHandlerMap("SCardListReaders",
                         &PcscLiteClientRequestProcessor::SCardListReaders);
  AddHandlerToHandlerMap(
      "SCardListReaderGroups",
      &PcscLiteClientRequestProcessor::SCardListReaderGroups);
  AddHandlerToHandlerMap("SCardCancel",
                         &PcscLiteClientRequestProcessor::SCardCancel);
  AddHandlerToHandlerMap("SCardIsValidContext",
                         &PcscLiteClientRequestProcessor::SCardIsValidContext);
}

template <typename... Args>
void PcscLiteClientRequestProcessor::AddHandlerToHandlerMap(
    const std::string& name,
    GenericRequestResult (PcscLiteClientRequestProcessor::*handler)(
        Args... args)) {
  const Handler wrapped_handler =
      WrapHandler<std::tuple<typename std::decay<Args>::type...>>(
          name, handler, MakeArgIndexes<sizeof...(Args)>());
  GOOGLE_SMART_CARD_CHECK(handler_map_.emplace(name, wrapped_handler).second);
}

template <typename ArgsTuple, typename F, size_t... arg_indexes>
PcscLiteClientRequestProcessor::Handler
PcscLiteClientRequestProcessor::WrapHandler(
    const std::string& name,
    F handler,
    ArgIndexes<arg_indexes...> /*unused*/) {
  return [this, name,
          handler](std::vector<Value> arguments) -> GenericRequestResult {
    ArgsTuple args_tuple;
    std::string error_message;
    RemoteCallArgumentsExtractor extractor(name, std::move(arguments));
    extractor.Extract(&std::get<arg_indexes>(args_tuple)...);
    if (!extractor.Finish())
      return ReturnFailure(extractor.error_message());
    return (this->*handler)(std::get<arg_indexes>(args_tuple)...);
  };
}

void PcscLiteClientRequestProcessor::ScheduleHandlesCleanup() {
  const std::vector<SCARDCONTEXT> s_card_contexts =
      s_card_handles_registry_.PopAllContexts();

  // The actual cleanup happens in a separate background thread, as the involved
  // SCard* functions may call blocking libusb* functions - which are not
  // allowed to be called from the main thread (attempting to do this will
  // result in deadlock).
  std::thread(&CleanupHandles, logging_prefix_, s_card_contexts).detach();
}

GenericRequestResult PcscLiteClientRequestProcessor::FindHandlerAndCall(
    const std::string& function_name,
    std::vector<Value> arguments) {
  const HandlerMap::const_iterator handler_map_iter =
      handler_map_.find(function_name);
  if (handler_map_iter == handler_map_.end()) {
    return ReturnFailure(
        FormatPrintfTemplate("Unknown function \"%s\"", function_name.c_str()));
  }
  const Handler& handler = handler_map_iter->second;

  GenericRequestResult result = handler(std::move(arguments));
  if (!result.is_successful()) {
    return ReturnFailure(FormatPrintfTemplate(
        "Error while processing the \"%s\" request: %s", function_name.c_str(),
        result.error_message().c_str()));
  }
  return result;
}

GenericRequestResult PcscLiteClientRequestProcessor::PcscLiteVersionNumber() {
  FunctionCallTracer tracer("PCSCLITE_VERSION_NUMBER", logging_prefix_,
                            status_log_severity_);
  tracer.LogEntrance();

  tracer.AddReturnValue(DebugDumpSCardCString(PCSCLITE_VERSION_NUMBER));
  tracer.LogExit();

  return ReturnValues(PCSCLITE_VERSION_NUMBER);
}

GenericRequestResult PcscLiteClientRequestProcessor::PcscStringifyError(
    LONG error) {
  FunctionCallTracer tracer("pcsc_stringify_error", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("pcscError", DebugDumpSCardReturnCode(error));
  tracer.LogEntrance();

  const char* const result = ::pcsc_stringify_error(error);

  tracer.AddReturnValue(DebugDumpSCardCString(result));
  tracer.LogExit();

  return ReturnValues(result);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardEstablishContext(
    DWORD scope,
    std::nullptr_t reserved_1,
    std::nullptr_t reserved_2) {
  FunctionCallTracer tracer("SCardEstablishContext", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("dwScope", DebugDumpSCardScope(scope));
  tracer.AddPassedArg("pvReserved1", Value::kNullTypeTitle);
  tracer.AddPassedArg("pvReserved2", Value::kNullTypeTitle);
  tracer.LogEntrance();

  SCARDCONTEXT s_card_context;
  const LONG return_code =
      ::SCardEstablishContext(scope, nullptr, nullptr, &s_card_context);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS)
    tracer.AddReturnedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  s_card_handles_registry_.AddContext(s_card_context);
  return ReturnValues(return_code, s_card_context);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardReleaseContext(
    SCARDCONTEXT s_card_context) {
  FunctionCallTracer tracer("SCardReleaseContext", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard("SCardReleaseContext",
                                           s_card_context, *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsContext(s_card_context))
    return_code = SCARD_E_INVALID_HANDLE;

  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardReleaseContext(s_card_context);

    // Catch when PC/SC-Lite core, unlike us, thinks the context doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardContextRevoked(s_card_context);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  if (return_code == SCARD_S_SUCCESS)
    s_card_handles_registry_.RemoveContext(s_card_context);

  return ReturnValues(return_code);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardConnect(
    SCARDCONTEXT s_card_context,
    const std::string& reader_name,
    DWORD share_mode,
    DWORD preferred_protocols) {
  FunctionCallTracer tracer("SCardConnect", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.AddPassedArg("szReader", '"' + reader_name + '"');
  tracer.AddPassedArg("dwShareMode", DebugDumpSCardShareMode(share_mode));
  tracer.AddPassedArg("dwPreferredProtocols",
                      DebugDumpSCardProtocols(preferred_protocols));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard("SCardConnect", s_card_context,
                                           *this);

  SCARDHANDLE s_card_handle;
  DWORD active_protocol;
  LONG return_code = ObtainCardHandleWithFallback(
      s_card_context, reader_name, share_mode, preferred_protocols,
      &s_card_handle, &active_protocol);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg("hCard", DebugDumpSCardHandle(s_card_handle));
    tracer.AddReturnedArg("dwActiveProtocol",
                          DebugDumpSCardProtocol(active_protocol));
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);

  return ReturnValues(return_code, s_card_handle, active_protocol);
}

LONG PcscLiteClientRequestProcessor::ObtainCardHandleWithFallback(
    SCARDCONTEXT s_card_context,
    const std::string& reader_name,
    DWORD share_mode,
    DWORD preferred_protocols,
    LPSCARDHANDLE s_card_handle,
    LPDWORD active_protocol) {
  LONG return_code =
      ObtainCardHandle(s_card_context, reader_name, share_mode,
                       preferred_protocols, s_card_handle, active_protocol);

  // If SCardConnect fails with SCARD_E_PROTO_MISMATCH it could be because a
  // client application did not correctly reset the card and is now trying to
  // reuse the previous connection but requires a different protocol. If allowed
  // via policy, disconnect and reset the card, and retry connecting to it. If
  // the fallback is not enabled, return the original error.
  if (return_code != SCARD_E_PROTO_MISMATCH ||
      !IsDisconnectFallbackPolicyEnabled()) {
    return return_code;
  }

  GOOGLE_SMART_CARD_LOG_INFO
      << logging_prefix_ << "SCardConnect failed with a protocol mismatch "
      << "error. Attempting SCardDisconnect fallback: disconnecting and "
      << "resetting any previous connections for context "
      << DebugDumpSCardContext(s_card_context);
  if (ResetCard(s_card_context, reader_name, share_mode) &&
      ObtainCardHandle(s_card_context, reader_name, share_mode,
                       preferred_protocols, s_card_handle,
                       active_protocol) == SCARD_S_SUCCESS) {
    return SCARD_S_SUCCESS;
  }

  // The fallback failed, so return the original error.
  return return_code;
}

LONG PcscLiteClientRequestProcessor::ObtainCardHandle(
    SCARDCONTEXT s_card_context,
    const std::string& reader_name,
    DWORD share_mode,
    DWORD preferred_protocols,
    LPSCARDHANDLE s_card_handle,
    LPDWORD active_protocol) {
  if (!s_card_handles_registry_.ContainsContext(s_card_context))
    return SCARD_E_INVALID_HANDLE;

  LONG return_code =
      ::SCardConnect(s_card_context, reader_name.c_str(), share_mode,
                     preferred_protocols, s_card_handle, active_protocol);
  if (return_code == SCARD_S_SUCCESS)
    s_card_handles_registry_.AddHandle(s_card_context, *s_card_handle);

  // Catch when PC/SC-Lite core, unlike us, thinks the context doesn't exist.
  if (return_code == SCARD_E_INVALID_HANDLE)
    OnSCardContextRevoked(s_card_context);

  return return_code;
}

bool PcscLiteClientRequestProcessor::IsDisconnectFallbackPolicyEnabled() {
  optional<AdminPolicy> admin_policy = admin_policy_getter_->WaitAndGet();
  std::vector<std::string> client_ids;
  if (admin_policy &&
      admin_policy.value().scard_disconnect_fallback_client_app_ids) {
    client_ids =
        admin_policy.value().scard_disconnect_fallback_client_app_ids.value();
  }

  return std::find(client_ids.begin(), client_ids.end(),
                   client_name_for_log_) != client_ids.end();
}

bool PcscLiteClientRequestProcessor::ResetCard(SCARDCONTEXT s_card_context,
                                               const std::string& reader_name,
                                               DWORD share_mode) {
  // Try to get an s_card_handle by connecting using any protocol.
  DWORD preferred_protocols =
      SCARD_PROTOCOL_ANY | SCARD_PROTOCOL_RAW | SCARD_PROTOCOL_T15;

  SCARDHANDLE s_card_handle = 0;
  DWORD active_protocol;
  if (ObtainCardHandle(s_card_context, reader_name, share_mode,
                       preferred_protocols, &s_card_handle,
                       &active_protocol) != SCARD_S_SUCCESS) {
    return false;
  }
  return DisconnectCard(s_card_handle, SCARD_RESET_CARD) == SCARD_S_SUCCESS;
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardReconnect(
    SCARDHANDLE s_card_handle,
    DWORD share_mode,
    DWORD preferred_protocols,
    DWORD initialization_action) {
  FunctionCallTracer tracer("SCardReconnect", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.AddPassedArg("dwShareMode", DebugDumpSCardShareMode(share_mode));
  tracer.AddPassedArg("dwPreferredProtocols",
                      DebugDumpSCardProtocols(preferred_protocols));
  tracer.AddPassedArg("dwInitialization",
                      DebugDumpSCardDisposition(initialization_action));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardReconnect",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  DWORD active_protocol;
  if (return_code == SCARD_S_SUCCESS) {
    return_code =
        ::SCardReconnect(s_card_handle, share_mode, preferred_protocols,
                         initialization_action, &active_protocol);

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg("dwActiveProtocol",
                          DebugDumpSCardProtocol(active_protocol));
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  return ReturnValues(return_code, active_protocol);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardDisconnect(
    SCARDHANDLE s_card_handle,
    DWORD disposition_action) {
  FunctionCallTracer tracer("SCardDisconnect", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.AddPassedArg("dwDisposition",
                      DebugDumpSCardDisposition(disposition_action));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardDisconnect",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = DisconnectCard(s_card_handle, disposition_action);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return ReturnValues(return_code);
}

LONG PcscLiteClientRequestProcessor::DisconnectCard(SCARDHANDLE s_card_handle,
                                                    DWORD disposition_action) {
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return SCARD_E_INVALID_HANDLE;

  LONG return_code = ::SCardDisconnect(s_card_handle, disposition_action);

  // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
  if (return_code == SCARD_E_INVALID_HANDLE)
    OnSCardHandleRevoked(s_card_handle);

  if (return_code == SCARD_S_SUCCESS)
    s_card_handles_registry_.RemoveHandle(s_card_handle);

  return return_code;
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardBeginTransaction(
    SCARDHANDLE s_card_handle) {
  FunctionCallTracer tracer("SCardBeginTransaction", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardBeginTransaction",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardBeginTransaction(s_card_handle);

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return ReturnValues(return_code);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardEndTransaction(
    SCARDHANDLE s_card_handle,
    DWORD disposition_action) {
  FunctionCallTracer tracer("SCardEndTransaction", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.AddPassedArg("dwDisposition",
                      DebugDumpSCardDisposition(disposition_action));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardEndTransaction",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardEndTransaction(s_card_handle, disposition_action);

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return ReturnValues(return_code);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardStatus(
    SCARDHANDLE s_card_handle) {
  FunctionCallTracer tracer("SCardStatus", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardStatus",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  LPSTR reader_name;
  DWORD reader_name_length = SCARD_AUTOALLOCATE;
  DWORD state;
  DWORD protocol;
  LPBYTE atr;
  DWORD atr_length = SCARD_AUTOALLOCATE;
  if (return_code == SCARD_S_SUCCESS) {
    return_code =
        ::SCardStatus(s_card_handle, reinterpret_cast<LPSTR>(&reader_name),
                      &reader_name_length, &state, &protocol,
                      reinterpret_cast<LPBYTE>(&atr), &atr_length);

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg("szReaderName", DebugDumpSCardCString(reader_name));
    tracer.AddReturnedArg("dwState", DebugDumpSCardState(state));
    tracer.AddReturnedArg("dwProtocol", DebugDumpSCardProtocol(protocol));
    tracer.AddReturnedArg("bAtr", "<" + HexDumpBytes(atr, atr_length) + ">");
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  const std::string reader_name_copy = reader_name;
  FreeSCardMemory(reader_name);
  const std::vector<uint8_t> atr_copy(atr, atr + atr_length);
  FreeSCardMemory(atr);
  return ReturnValues(return_code, reader_name_copy, state, protocol, atr_copy);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardGetStatusChange(
    SCARDCONTEXT s_card_context,
    DWORD timeout,
    const std::vector<InboundSCardReaderState>& reader_states) {
  std::vector<SCARD_READERSTATE> pcsc_lite_reader_states;
  for (const InboundSCardReaderState& reader_state : reader_states) {
    SCARD_READERSTATE current_item;
    std::memset(&current_item, 0, sizeof(current_item));

    // Note: a pointer to the std::string contents is stored in the structure
    // here. This is OK as the scope of the created SCARD_READERSTATE structures
    // is enclosed into this function body.
    current_item.szReader = reader_state.reader_name.c_str();

    if (reader_state.user_data) {
      GOOGLE_SMART_CARD_CHECK(*reader_state.user_data);
      current_item.pvUserData =
          reinterpret_cast<void*>(*reader_state.user_data);
    }

    current_item.dwCurrentState = reader_state.current_state;

    pcsc_lite_reader_states.push_back(current_item);
  }

  FunctionCallTracer tracer("SCardGetStatusChange", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.AddPassedArg("dwTimeout", std::to_string(timeout));
  tracer.AddPassedArg("rgReaderStates", DebugDumpSCardInputReaderStates(
                                            pcsc_lite_reader_states.empty()
                                                ? nullptr
                                                : &pcsc_lite_reader_states[0],
                                            pcsc_lite_reader_states.size()));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard("SCardGetStatusChange",
                                           s_card_context, *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsContext(s_card_context))
    return_code = SCARD_E_INVALID_HANDLE;

  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardGetStatusChange(
        s_card_context, timeout,
        pcsc_lite_reader_states.empty() ? nullptr : &pcsc_lite_reader_states[0],
        pcsc_lite_reader_states.size());

    // Catch when PC/SC-Lite core, unlike us, thinks the context doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardContextRevoked(s_card_context);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg(
        "rgReaderStates",
        DebugDumpSCardOutputReaderStates(pcsc_lite_reader_states.empty()
                                             ? nullptr
                                             : &pcsc_lite_reader_states[0],
                                         pcsc_lite_reader_states.size()));
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);

  std::vector<OutboundSCardReaderState> result_reader_states;
  for (const SCARD_READERSTATE& reader_state : pcsc_lite_reader_states) {
    result_reader_states.push_back(
        OutboundSCardReaderState::FromSCardReaderState(reader_state));
  }
  return ReturnValues(return_code, result_reader_states);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardControl(
    SCARDHANDLE s_card_handle,
    DWORD control_code,
    const std::vector<uint8_t>& data_to_send) {
  FunctionCallTracer tracer("SCardControl", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.AddPassedArg("dwControlCode", DebugDumpSCardControlCode(control_code));
  tracer.AddPassedArg("bSendBuffer",
                      "<" + DebugDumpSCardBufferContents(data_to_send) + ">");
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardControl",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  DWORD bytes_received;
  std::vector<uint8_t> buffer(MAX_BUFFER_SIZE_EXTENDED);
  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardControl(s_card_handle, control_code, &data_to_send[0],
                                 data_to_send.size(), &buffer[0], buffer.size(),
                                 &bytes_received);

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }
  if (return_code == SCARD_S_SUCCESS)
    buffer.resize(bytes_received);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg(
        "bRecvBuffer",
        "<" + DebugDumpSCardBufferContents(buffer.data(), bytes_received) +
            ">");
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  return ReturnValues(return_code, buffer);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardGetAttrib(
    SCARDHANDLE s_card_handle,
    DWORD attribute_id) {
  FunctionCallTracer tracer("SCardGetAttrib", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.AddPassedArg("dwAttrId", DebugDumpSCardAttributeId(attribute_id));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardGetAttrib",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  LPBYTE attribute;
  DWORD attribute_length = SCARD_AUTOALLOCATE;
  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardGetAttrib(s_card_handle, attribute_id,
                                   reinterpret_cast<LPBYTE>(&attribute),
                                   &attribute_length);

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg(
        "bAttr", "<" + HexDumpBytes(attribute, attribute_length) + ">");
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  const std::vector<uint8_t> attribute_copy(attribute,
                                            attribute + attribute_length);
  FreeSCardMemory(attribute);
  return ReturnValues(return_code, attribute_copy);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardSetAttrib(
    SCARDHANDLE s_card_handle,
    DWORD attribute_id,
    const std::vector<uint8_t>& attribute) {
  FunctionCallTracer tracer("SCardSetAttrib", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.AddPassedArg("dwAttrId", DebugDumpSCardAttributeId(attribute_id));
  tracer.AddPassedArg("pbAttr", "<" + HexDumpBytes(attribute) + ">");
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardSetAttrib",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardSetAttrib(s_card_handle, attribute_id,
                                   attribute.empty() ? nullptr : &attribute[0],
                                   attribute.size());

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return ReturnValues(return_code);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardTransmit(
    SCARDHANDLE s_card_handle,
    const SCardIoRequest& send_protocol_information,
    const std::vector<uint8_t>& data_to_send,
    const optional<SCardIoRequest>& response_protocol_information) {
  const SCARD_IO_REQUEST scard_send_protocol_information =
      send_protocol_information.AsSCardIoRequest();
  SCARD_IO_REQUEST scard_response_protocol_information;
  if (response_protocol_information) {
    scard_response_protocol_information =
        response_protocol_information->AsSCardIoRequest();
  } else {
    scard_response_protocol_information.dwProtocol = SCARD_PROTOCOL_ANY;
    scard_response_protocol_information.cbPciLength = sizeof(SCARD_IO_REQUEST);
  }

  FunctionCallTracer tracer("SCardTransmit", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(s_card_handle));
  tracer.AddPassedArg("ioSendPci",
                      DebugDumpSCardIoRequest(scard_send_protocol_information));
  tracer.AddPassedArg("pbSendBuffer",
                      "<" + DebugDumpSCardBufferContents(data_to_send) + ">");
  if (response_protocol_information) {
    tracer.AddPassedArg("ioRecvPci", DebugDumpSCardIoRequest(
                                         scard_response_protocol_information));
  }
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard(
      "SCardTransmit",
      s_card_handles_registry_.FindContextByHandle(s_card_handle), *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsHandle(s_card_handle))
    return_code = SCARD_E_INVALID_HANDLE;

  std::vector<uint8_t> buffer(MAX_BUFFER_SIZE_EXTENDED);
  DWORD response_length = buffer.size();
  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardTransmit(
        s_card_handle, &scard_send_protocol_information,
        data_to_send.empty() ? nullptr : &data_to_send[0], data_to_send.size(),
        &scard_response_protocol_information, &buffer[0], &response_length);

    // Catch when PC/SC-Lite core, unlike us, thinks the handle doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardHandleRevoked(s_card_handle);
  }
  if (return_code == SCARD_S_SUCCESS)
    buffer.resize(response_length);

  if (!response_protocol_information &&
      scard_response_protocol_information.dwProtocol == SCARD_PROTOCOL_ANY) {
    // When SCARD_PROTOCOL_ANY placeholder value was passed to SCardTransmit as
    // the value of pioRecvPci->dwProtocol, it may be returned (and IS actually
    // returned with the current implementation of PC/SC-Lite and CCID)
    // unmodified - and that's technically correct, as such usage is not
    // officially documented for PC/SC-Lite. (They actually do the similar
    // placeholder substitution internally, but only when no input parameter was
    // passed - therefore without any effect on the output arguments.)
    //
    // But as this NaCl port always returns the value of this output argument to
    // the callers, even when the caller didn't supply the input parameter with
    // the protocol, then this SCARD_PROTOCOL_ANY placeholder value has to be
    // replaced with some actual protocol value. There is no absolutely reliable
    // way to obtain it here, but assuming that it's the same as the input
    // protocol seems to be rather safe.
    scard_response_protocol_information.dwProtocol =
        scard_send_protocol_information.dwProtocol;
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg(
        "ioRecvPci",
        DebugDumpSCardIoRequest(scard_response_protocol_information));
    tracer.AddReturnedArg(
        "bRecvBuffer",
        "<" + DebugDumpSCardBufferContents(buffer.data(), response_length) +
            ">");
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  return ReturnValues(
      return_code,
      SCardIoRequest::FromSCardIoRequest(scard_response_protocol_information),
      buffer);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardListReaders(
    SCARDCONTEXT s_card_context,
    std::nullptr_t groups) {
  FunctionCallTracer tracer("SCardListReaders", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.AddPassedArg("mszGroups", Value::kNullTypeTitle);
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard("SCardListReaders", s_card_context,
                                           *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsContext(s_card_context))
    return_code = SCARD_E_INVALID_HANDLE;

  LPSTR readers;
  DWORD readers_length = SCARD_AUTOALLOCATE;
  if (return_code == SCARD_S_SUCCESS) {
    return_code =
        ::SCardListReaders(s_card_context, nullptr,
                           reinterpret_cast<LPSTR>(&readers), &readers_length);

    // Catch when PC/SC-Lite core, unlike us, thinks the context doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardContextRevoked(s_card_context);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS)
    tracer.AddReturnedArg("mszReaders", DebugDumpSCardMultiString(readers));
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  const std::vector<std::string> readers_list =
      ExtractMultiStringElements(readers);
  FreeSCardMemory(readers);
  return ReturnValues(return_code, readers_list);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardListReaderGroups(
    SCARDCONTEXT s_card_context) {
  FunctionCallTracer tracer("SCardListReaderGroups", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard("SCardListReaderGroups",
                                           s_card_context, *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsContext(s_card_context))
    return_code = SCARD_E_INVALID_HANDLE;

  LPSTR reader_groups;
  DWORD reader_groups_length = SCARD_AUTOALLOCATE;
  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardListReaderGroups(
        s_card_context, reinterpret_cast<LPSTR>(&reader_groups),
        &reader_groups_length);

    // Catch when PC/SC-Lite core, unlike us, thinks the context doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardContextRevoked(s_card_context);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg("*mszGroups",
                          DebugDumpSCardMultiString(reader_groups));
  }
  tracer.LogExit();

  if (return_code != SCARD_S_SUCCESS)
    return ReturnValues(return_code);
  const std::vector<std::string> reader_groups_list =
      ExtractMultiStringElements(reader_groups);
  FreeSCardMemory(reader_groups);
  return ReturnValues(return_code, reader_groups_list);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardCancel(
    SCARDCONTEXT s_card_context) {
  FunctionCallTracer tracer("SCardCancel", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.LogEntrance();
  // Note there's no `ScopedConcurrencyGuard`, since PC/SC API allows calling
  // `SCardCancel()` from different threads.

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsContext(s_card_context))
    return_code = SCARD_E_INVALID_HANDLE;

  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardCancel(s_card_context);

    // Catch when PC/SC-Lite core, unlike us, thinks the context doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardContextRevoked(s_card_context);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return ReturnValues(return_code);
}

GenericRequestResult PcscLiteClientRequestProcessor::SCardIsValidContext(
    SCARDCONTEXT s_card_context) {
  FunctionCallTracer tracer("SCardIsValidContext", logging_prefix_,
                            status_log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(s_card_context));
  tracer.LogEntrance();
  ScopedConcurrencyGuard concurrency_guard("SCardIsValidContext",
                                           s_card_context, *this);

  LONG return_code = SCARD_S_SUCCESS;
  if (!s_card_handles_registry_.ContainsContext(s_card_context))
    return_code = SCARD_E_INVALID_HANDLE;

  if (return_code == SCARD_S_SUCCESS) {
    return_code = ::SCardIsValidContext(s_card_context);

    // Catch when PC/SC-Lite core, unlike us, thinks the context doesn't exist.
    if (return_code == SCARD_E_INVALID_HANDLE)
      OnSCardContextRevoked(s_card_context);
  }

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return ReturnValues(return_code);
}

void PcscLiteClientRequestProcessor::OnSCardContextRevoked(
    SCARDCONTEXT s_card_context) {
  GOOGLE_SMART_CARD_LOG_WARNING
      << "PC/SC-Lite unexpectedly revoked the context "
      << DebugDumpSCardContext(s_card_context);
  s_card_handles_registry_.RemoveContext(s_card_context);
}

void PcscLiteClientRequestProcessor::OnSCardHandleRevoked(
    SCARDHANDLE s_card_handle) {
  GOOGLE_SMART_CARD_LOG_WARNING << "PC/SC-Lite unexpectedly revoked the handle "
                                << DebugDumpSCardHandle(s_card_handle);
  s_card_handles_registry_.RemoveHandle(s_card_handle);
}

}  // namespace google_smart_card
