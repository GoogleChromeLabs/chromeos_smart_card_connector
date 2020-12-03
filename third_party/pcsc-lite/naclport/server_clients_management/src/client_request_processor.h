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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENT_REQUEST_PROCESSOR_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENT_REQUEST_PROCESSOR_H_

#include <stdint.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_receiver.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/tuple_unpacking.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_pcsc_lite_common/scard_structs_serialization.h>

#include "client_handles_registry.h"

namespace google_smart_card {

// This class corresponds to a single external PC/SC-Lite client. It executes
// PC/SC-Lite API requests received from the client, keeps tracking the handles
// opened by the client and checks that client accesses only these handles.
//
// This class is an important piece for providing privacy and security of the
// PC/SC-Lite NaCl port: it ensures that the client is isolated from all other
// clients. Every handle specified in client's request is examined and checked
// to be belonging to this client. Without these checks, the low-level
// PC/SC-Lite API would accept any handle, which would allow one client to
// interfere with other clients.
//
// Apart from providing security/privacy checks, instance of this class performs
// the actual execution of PC/SC-Lite API requests received from the client. The
// class provides a method of asynchronous request execution, which happens in
// background threads. There may be multiple requests being executed
// simultaneously. (And even the same SCARDCONTEXT may be used legally in two
// or more simultaneous requests: for example, an "SCardGetStatusChange" request
// and an "SCardCancel" request.)
//
// Additionally, keeping track of all opened handles allows to perform a proper
// cleanup when the external client disconnects without doing this (for example,
// when the external client crashes).
//
// The class has a refcounting-based storage, which allows to postpone the class
// instance destruction until the last running request finishes. FIXME(emaxx):
// Drop this requirement using the WeakPtrFactory concept (inspired by the
// Chromium source code).
//
// FIXME(emaxx): The class should be re-designed to provide a more secure and
// robust implementation that would be based a stricter threading model: all
// requests for a given PC/SC-Lite context should be executed sequentially on
// the same worker thread. Apart from following the PC/SC-Lite API contract
// (which requests this threading model: no more than one thread per context,
// with the exception of SCardCancel requests), this would also handle the
// theoretically possible race between releasing contexts in one request
// processor and gaining them in the other one. The safety of the current
// implementation relies on the PC/SC-Lite not generating the identical contexts
// too soon.
//
// FIXME(emaxx): Add assertions that the methods are called on the right
// threads.
class PcscLiteClientRequestProcessor final
    : public std::enable_shared_from_this<PcscLiteClientRequestProcessor> {
 public:
  PcscLiteClientRequestProcessor(int64_t client_handler_id,
                                 const optional<std::string>& client_app_id);
  PcscLiteClientRequestProcessor(const PcscLiteClientRequestProcessor&) =
      delete;
  PcscLiteClientRequestProcessor& operator=(
      const PcscLiteClientRequestProcessor&) = delete;

  ~PcscLiteClientRequestProcessor();

  // Schedules a cancellation of long-running PC/SC-Lite requests to be
  // performed in a background thread.
  //
  // Note that only the SCardGetStatusChange requests support cancellation, all
  // other requests will continue working till their normal finish.
  //
  // This method is safe to be called from any thread.
  void ScheduleRunningRequestsCancellation();

  // Process the given PC/SC-Lite request.
  //
  // The result is returned through the passed callback immediately (before this
  // method returns).
  //
  // This method is safe to be called from any thread, except the main Pepper
  // thread (which could lead to a deadlock).
  void ProcessRequest(RemoteCallRequestPayload request,
                      RequestReceiver::ResultCallback result_callback);

  // Start processing the given PC/SC-Lite request in a background thread.
  static void AsyncProcessRequest(
      std::shared_ptr<PcscLiteClientRequestProcessor> request_processor,
      RemoteCallRequestPayload request,
      RequestReceiver::ResultCallback result_callback);

 private:
  using Handler =
      std::function<GenericRequestResult(std::vector<Value> arguments)>;
  using HandlerMap = std::unordered_map<std::string, Handler>;

  void BuildHandlerMap();

  template <typename... Args>
  void AddHandlerToHandlerMap(
      const std::string& name,
      GenericRequestResult (PcscLiteClientRequestProcessor::*handler)(
          Args... args));

  template <typename ArgsTuple, typename F, size_t... arg_indexes>
  Handler WrapHandler(const std::string& name,
                      F handler,
                      ArgIndexes<arg_indexes...> /*unused*/);

  GenericRequestResult FindHandlerAndCall(const std::string& function_name,
                                          std::vector<Value> arguments);

  void ScheduleHandlesCleanup();

  GenericRequestResult PcscLiteVersionNumber();
  GenericRequestResult PcscStringifyError(LONG error);
  GenericRequestResult SCardEstablishContext(DWORD scope,
                                             std::nullptr_t reserved_1,
                                             std::nullptr_t reserved_2);
  GenericRequestResult SCardReleaseContext(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardConnect(SCARDCONTEXT s_card_context,
                                    const std::string& reader_name,
                                    DWORD share_mode,
                                    DWORD preferred_protocols);
  GenericRequestResult SCardReconnect(SCARDHANDLE s_card_handle,
                                      DWORD share_mode,
                                      DWORD preferred_protocols,
                                      DWORD initialization_action);
  GenericRequestResult SCardDisconnect(SCARDHANDLE s_card_handle,
                                       DWORD disposition_action);
  GenericRequestResult SCardBeginTransaction(SCARDHANDLE s_card_handle);
  GenericRequestResult SCardEndTransaction(SCARDHANDLE s_card_handle,
                                           DWORD disposition_action);
  GenericRequestResult SCardStatus(SCARDHANDLE s_card_handle);
  GenericRequestResult SCardGetStatusChange(
      SCARDCONTEXT s_card_context,
      DWORD timeout,
      const std::vector<InboundSCardReaderState>& reader_states);
  GenericRequestResult SCardControl(SCARDHANDLE s_card_handle,
                                    DWORD control_code,
                                    const std::vector<uint8_t>& data_to_send);
  GenericRequestResult SCardGetAttrib(SCARDHANDLE s_card_handle,
                                      DWORD attribute_id);
  GenericRequestResult SCardSetAttrib(SCARDHANDLE s_card_handle,
                                      DWORD attribute_id,
                                      const std::vector<uint8_t>& attribute);
  GenericRequestResult SCardTransmit(
      SCARDHANDLE s_card_handle,
      const SCardIoRequest& send_protocol_information,
      const std::vector<uint8_t>& data_to_send,
      const optional<SCardIoRequest>& response_protocol_information);
  GenericRequestResult SCardListReaders(SCARDCONTEXT s_card_context,
                                        std::nullptr_t groups);
  GenericRequestResult SCardListReaderGroups(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardCancel(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardIsValidContext(SCARDCONTEXT s_card_context);

  const int64_t client_handler_id_;
  const optional<std::string> client_app_id_;
  const LogSeverity status_log_severity_;
  const std::string logging_prefix_;
  HandlerMap handler_map_;
  PcscLiteClientHandlesRegistry s_card_handles_registry_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENT_REQUEST_PROCESSOR_H_
