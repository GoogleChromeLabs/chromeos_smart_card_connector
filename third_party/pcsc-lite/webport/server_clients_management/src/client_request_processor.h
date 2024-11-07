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
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/requesting/request_receiver.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/tuple_unpacking.h"
#include "common/cpp/src/public/value.h"
#include "third_party/pcsc-lite/webport/common/src/public/scard_structs_serialization.h"
#include "third_party/pcsc-lite/webport/server_clients_management/src/admin_policy_getter.h"
#include "third_party/pcsc-lite/webport/server_clients_management/src/client_handles_registry.h"

namespace google_smart_card {

// This class corresponds to a single external PC/SC-Lite client. It executes
// PC/SC-Lite API requests received from the client, keeps tracking the handles
// opened by the client and checks that client accesses only these handles.
//
// This class is an important piece for providing privacy and security of the
// PC/SC-Lite web port: it ensures that the client is isolated from all other
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
  // `client_handler_id` - a number that uniquely identifies a handler (note
  // that a single client application might open multiple connections to us,
  // each of which will have a separate handler).
  // `client_name_for_log` - a name describing the client for logging purposes,
  // or an empty string if it's our own application talking to itself.
  PcscLiteClientRequestProcessor(int64_t client_handler_id,
                                 const std::string& client_name_for_log,
                                 AdminPolicyGetter* admin_policy_getter);
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

  // Helper class for making updates to `context_to_running_functions_`. On
  // construction, adds the given function name and logs a warning if misuse
  // detected. On destruction, undoes the change.
  class ScopedConcurrencyGuard final {
   public:
    ScopedConcurrencyGuard(const std::string& function_name,
                           SCARDCONTEXT s_card_context,
                           PcscLiteClientRequestProcessor& owner);

    ScopedConcurrencyGuard(const ScopedConcurrencyGuard&) = delete;
    ScopedConcurrencyGuard& operator=(const ScopedConcurrencyGuard&) = delete;

    ~ScopedConcurrencyGuard();

   private:
    const std::string function_name_;
    const SCARDCONTEXT s_card_context_;
    PcscLiteClientRequestProcessor& owner_;
  };

  friend class ScopedConcurrencyGuard;

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
  LONG ObtainCardHandleWithFallback(SCARDCONTEXT s_card_context,
                                    const std::string& reader_name,
                                    DWORD share_mode,
                                    DWORD preferred_protocols,
                                    LPSCARDHANDLE s_card_handle,
                                    LPDWORD active_protocol);
  LONG ObtainCardHandle(SCARDCONTEXT s_card_context,
                        const std::string& reader_name,
                        DWORD share_mode,
                        DWORD preferred_protocols,
                        LPSCARDHANDLE s_card_handle,
                        LPDWORD active_protocol);
  bool IsDisconnectFallbackPolicyEnabled();
  bool ResetCard(SCARDCONTEXT s_card_context,
                 const std::string& reader_name,
                 DWORD share_mode);
  GenericRequestResult SCardReconnect(SCARDHANDLE s_card_handle,
                                      DWORD share_mode,
                                      DWORD preferred_protocols,
                                      DWORD initialization_action);
  GenericRequestResult SCardDisconnect(SCARDHANDLE s_card_handle,
                                       DWORD disposition_action);
  LONG DisconnectCard(SCARDHANDLE s_card_handle, DWORD disposition_action);
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
      const std::optional<SCardIoRequest>& response_protocol_information);
  GenericRequestResult SCardListReaders(SCARDCONTEXT s_card_context,
                                        std::nullptr_t groups);
  GenericRequestResult SCardListReaderGroups(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardCancel(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardIsValidContext(SCARDCONTEXT s_card_context);

  // These two methods are called when the PC/SC-Lite core reports the
  // context/handle doesn't exist, meanwhile our class thought it is. It
  // shouldn't happen normally, but we met such bugs in the past (see
  // <https://github.com/GoogleChromeLabs/chromeos_smart_card_connector/issues/681>).
  void OnSCardContextRevoked(SCARDCONTEXT s_card_context);
  void OnSCardHandleRevoked(SCARDHANDLE s_card_handle);

  const int64_t client_handler_id_;
  const std::string client_name_for_log_;
  const LogSeverity status_log_severity_;
  const std::string logging_prefix_;
  AdminPolicyGetter* const admin_policy_getter_;
  HandlerMap handler_map_;
  // Stores PC/SC-Lite contexts and handles that belong to this client. This is
  // used to implement the client isolation: one client shouldn't be able to use
  // contexts/handles belonging to the other one.
  PcscLiteClientHandlesRegistry s_card_handles_registry_;

  mutable std::mutex mutex_;
  std::unordered_map<SCARDCONTEXT, std::multiset<std::string>>
      context_to_running_functions_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENT_REQUEST_PROCESSOR_H_
