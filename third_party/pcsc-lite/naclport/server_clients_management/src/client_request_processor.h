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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/request_receiver.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/tuple_unpacking.h>
#include <google_smart_card_pcsc_lite_common/scard_structs_serialization.h>

#include "client_handles_registry.h"
#include "client_id.h"

namespace google_smart_card {

// This class corresponds to a single external PC/SC-Lite client. It executes
// PC/SC-Lite API requests received from the client, keeps tracking of the
// handles opened by the client and checks that client accesses only these
// handles.
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
// instance destruction until the last running request finishes.
class PcscLiteClientRequestProcessor final :
    public std::enable_shared_from_this<PcscLiteClientRequestProcessor> {
 public:
  explicit PcscLiteClientRequestProcessor(PcscLiteServerClientId client_id);

  ~PcscLiteClientRequestProcessor();

  void ProcessRequest(
      const std::string& function_name,
      const pp::VarArray& arguments,
      RequestReceiver::ResultCallback result_callback);

  static void AsyncProcessRequest(
      std::shared_ptr<PcscLiteClientRequestProcessor> request_processor,
      const std::string& function_name,
      const pp::VarArray& arguments,
      RequestReceiver::ResultCallback result_callback);

 private:
  using Handler =
      std::function<GenericRequestResult(const pp::VarArray& arguments)>;
  using HandlerMap = std::unordered_map<std::string, Handler>;

  void BuildHandlerMap();

  template <typename ... Args>
  void AddHandlerToHandlerMap(
      const std::string& name,
      GenericRequestResult(PcscLiteClientRequestProcessor::*handler)(
          Args ... args));

  template <typename ArgsTuple, typename F, size_t ... arg_indexes>
  Handler WrapHandler(F handler, ArgIndexes<arg_indexes...> /*unused*/);

  GenericRequestResult FindHandlerAndCall(
      const std::string& function_name, const pp::VarArray& arguments);

  void ScheduleClosingLeftHandles();
  static void CloseLeftHandles(
      const std::string& logging_prefix,
      const std::vector<SCARDCONTEXT>& s_card_contexts);

  GenericRequestResult PcscLiteVersionNumber();
  GenericRequestResult PcscStringifyError(LONG error);
  GenericRequestResult SCardEstablishContext(
      DWORD scope, pp::Var::Null reserved_1, pp::Var::Null reserved_2);
  GenericRequestResult SCardReleaseContext(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardConnect(
      SCARDCONTEXT s_card_context,
      const std::string& reader_name,
      DWORD share_mode,
      DWORD preferred_protocols);
  GenericRequestResult SCardReconnect(
      SCARDHANDLE s_card_handle,
      DWORD share_mode,
      DWORD preferred_protocols,
      DWORD initialization_action);
  GenericRequestResult SCardDisconnect(
      SCARDHANDLE s_card_handle, DWORD disposition_action);
  GenericRequestResult SCardBeginTransaction(SCARDHANDLE s_card_handle);
  GenericRequestResult SCardEndTransaction(
      SCARDHANDLE s_card_handle, DWORD disposition_action);
  GenericRequestResult SCardStatus(SCARDHANDLE s_card_handle);
  GenericRequestResult SCardGetStatusChange(
      SCARDCONTEXT s_card_context,
      DWORD timeout,
      const std::vector<InboundSCardReaderState>& reader_states);
  GenericRequestResult SCardControl(
      SCARDHANDLE s_card_handle,
      DWORD control_code,
      const std::vector<uint8_t>& data_to_send);
  GenericRequestResult SCardGetAttrib(
      SCARDHANDLE s_card_handle, DWORD attribute_id);
  GenericRequestResult SCardSetAttrib(
      SCARDHANDLE s_card_handle,
      DWORD attribute_id,
      const std::vector<uint8_t>& attribute);
  GenericRequestResult SCardTransmit(
      SCARDHANDLE s_card_handle,
      const SCardIoRequest& send_protocol_information,
      const std::vector<uint8_t>& data_to_send,
      const optional<SCardIoRequest>& response_protocol_information);
  GenericRequestResult SCardListReaders(
      SCARDCONTEXT s_card_context, pp::Var::Null groups);
  GenericRequestResult SCardListReaderGroups(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardCancel(SCARDCONTEXT s_card_context);
  GenericRequestResult SCardIsValidContext(SCARDCONTEXT s_card_context);

  const PcscLiteServerClientId client_id_;
  const std::string logging_prefix_;
  HandlerMap handler_map_;
  PcscLiteClientHandlesRegistry s_card_handles_registry_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENT_REQUEST_PROCESSOR_H_
