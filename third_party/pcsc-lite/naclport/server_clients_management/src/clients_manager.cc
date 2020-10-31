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

#include "clients_manager.h"

#include <inttypes.h>

#include <sstream>
#include <utility>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/struct_converter.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

#include "client_request_processor.h"

namespace google_smart_card {

namespace {

constexpr char kCreateHandlerMessageType[] = "pcsc_lite_create_client_handler";
constexpr char kDeleteHandlerMessageType[] = "pcsc_lite_delete_client_handler";
constexpr char kLoggingPrefix[] = "[PC/SC-Lite clients manager] ";

// The structure represents the message data contents for the client handler
// creation message.
struct CreateHandlerMessageData {
  int64_t handler_id;
  optional<std::string> client_app_id;
};

// The structure represents the message data contents for the client handler
// deletion message.
struct DeleteHandlerMessageData {
  int64_t handler_id;
};

}  // namespace

template <>
StructValueDescriptor<CreateHandlerMessageData>::Description
StructValueDescriptor<CreateHandlerMessageData>::GetDescription() {
  return Describe("CreateHandlerMessageData")
      .WithField(&CreateHandlerMessageData::handler_id, "handler_id")
      .WithField(&CreateHandlerMessageData::client_app_id, "client_app_id");
}

template <>
constexpr const char*
StructConverter<CreateHandlerMessageData>::GetStructTypeName() {
  return "CreateHandlerMessageData";
}

template <>
template <typename Callback>
void StructConverter<CreateHandlerMessageData>::VisitFields(
    const CreateHandlerMessageData& value, Callback callback) {
  callback(&value.handler_id, "handler_id");
  callback(&value.client_app_id, "client_app_id");
}

template <>
StructValueDescriptor<DeleteHandlerMessageData>::Description
StructValueDescriptor<DeleteHandlerMessageData>::GetDescription() {
  return Describe("DeleteHandlerMessageData")
      .WithField(&DeleteHandlerMessageData::handler_id, "handler_id");
}

template <>
constexpr const char*
StructConverter<DeleteHandlerMessageData>::GetStructTypeName() {
  return "DeleteHandlerMessageData";
}

template <>
template <typename Callback>
void StructConverter<DeleteHandlerMessageData>::VisitFields(
    const DeleteHandlerMessageData& value, Callback callback) {
  callback(&value.handler_id, "handler_id");
}

PcscLiteServerClientsManager::PcscLiteServerClientsManager(
    pp::Instance* pp_instance, TypedMessageRouter* typed_message_router)
    : pp_instance_(pp_instance),
      typed_message_router_(typed_message_router),
      create_handler_message_listener_(this),
      delete_handler_message_listener_(this) {
  GOOGLE_SMART_CARD_CHECK(pp_instance);
  GOOGLE_SMART_CARD_CHECK(typed_message_router);

  typed_message_router_->AddRoute(&create_handler_message_listener_);
  typed_message_router_->AddRoute(&delete_handler_message_listener_);
}

PcscLiteServerClientsManager::~PcscLiteServerClientsManager() { Detach(); }

void PcscLiteServerClientsManager::Detach() {
  GOOGLE_SMART_CARD_CHECK(typed_message_router_);
  typed_message_router_->RemoveRoute(&create_handler_message_listener_);
  typed_message_router_->RemoveRoute(&delete_handler_message_listener_);
  DeleteAllHandlers();
  pp_instance_ = nullptr;
  typed_message_router_ = nullptr;
}

PcscLiteServerClientsManager::CreateHandlerMessageListener::
    CreateHandlerMessageListener(PcscLiteServerClientsManager* clients_manager)
    : clients_manager_(clients_manager) {}

std::string PcscLiteServerClientsManager::CreateHandlerMessageListener::
    GetListenedMessageType() const {
  return kCreateHandlerMessageType;
}

bool PcscLiteServerClientsManager::CreateHandlerMessageListener::
    OnTypedMessageReceived(Value data) {
  // TODO: Parse `Value` directly instead of transforming into `pp::Var`.
  const pp::Var data_var = ConvertValueToPpVar(data);
  CreateHandlerMessageData message_data;
  std::string error_message;
  if (!StructConverter<CreateHandlerMessageData>::ConvertFromVar(
          data_var, &message_data, &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL
        << kLoggingPrefix << "Failed to parse "
        << "client handler creation message: " << error_message;
  }
  GOOGLE_SMART_CARD_CHECK(!message_data.client_app_id ||
                          !message_data.client_app_id->empty());

  clients_manager_->CreateHandler(message_data.handler_id,
                                  message_data.client_app_id);
  return true;
}

PcscLiteServerClientsManager::DeleteHandlerMessageListener ::
    DeleteHandlerMessageListener(PcscLiteServerClientsManager* clients_manager)
    : clients_manager_(clients_manager) {}

std::string PcscLiteServerClientsManager::DeleteHandlerMessageListener ::
    GetListenedMessageType() const {
  return kDeleteHandlerMessageType;
}

bool PcscLiteServerClientsManager::DeleteHandlerMessageListener ::
    OnTypedMessageReceived(Value data) {
  // TODO: Parse `Value` directly instead of transforming into `pp::Var`.
  const pp::Var data_var = ConvertValueToPpVar(data);
  DeleteHandlerMessageData message_data;
  std::string error_message;
  if (!StructConverter<DeleteHandlerMessageData>::ConvertFromVar(
          data_var, &message_data, &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL
        << kLoggingPrefix << "Failed to parse "
        << "client handler deletion message: " << error_message;
  }

  clients_manager_->DeleteHandler(message_data.handler_id);
  return true;
}

PcscLiteServerClientsManager::Handler::Handler(
    int64_t handler_id, const optional<std::string>& client_app_id,
    pp::Instance* pp_instance, TypedMessageRouter* typed_message_router)
    : handler_id_(handler_id),
      client_app_id_(client_app_id),
      request_processor_(
          new PcscLiteClientRequestProcessor(handler_id, client_app_id_)),
      request_receiver_(new JsRequestReceiver(
          FormatPrintfTemplate(
              "pcsc_lite_client_handler_%" PRId64 "_call_function", handler_id),
          this, typed_message_router,
          MakeUnique<JsRequestReceiver::PpDelegateImpl>(pp_instance))) {}

PcscLiteServerClientsManager::Handler::~Handler() {
  // Cancel long-running PC/SC-Lite requests that are currently processed by
  // this handler, to make it possible for a new handler to use the currently
  // occupied PC/SC-Lite resources. This is useful, for instance, when a client
  // is restarted and attempts to reestablish its state. Also this is absolutely
  // crucial in the cases when a potentially infinite-running request is
  // currently processed - otherwise there's a possibility that the PC/SC-Lite
  // resources would be blocked by the old, detached, handler forever.
  request_processor_->ScheduleRunningRequestsCancellation();
  // Stop receiving the new PC/SC-Lite requests from the JavaScript side, and
  // also disable sending of the request responses back to the JavaScript side.
  request_receiver_->Detach();
}

void PcscLiteServerClientsManager::Handler::HandleRequest(
    const pp::Var& payload, RequestReceiver::ResultCallback result_callback) {
  std::string function_name;
  pp::VarArray arguments;
  if (!ParseRemoteCallRequestPayload(payload, &function_name, &arguments)) {
    result_callback(GenericRequestResult::CreateFailed(
        "Failed to parse remote call request payload"));
    return;
  }

  PcscLiteClientRequestProcessor::AsyncProcessRequest(
      request_processor_, function_name, arguments, result_callback);
}

void PcscLiteServerClientsManager::CreateHandler(
    int64_t handler_id, const optional<std::string>& client_app_id) {
  std::unique_ptr<Handler> handler(new Handler(
      handler_id, client_app_id, pp_instance_, typed_message_router_));
  if (!handler_map_.emplace(handler_id, std::move(handler)).second) {
    GOOGLE_SMART_CARD_LOG_FATAL << kLoggingPrefix << "Failed to create a "
                                << "new client handler with id " << handler_id
                                << ": handler with this "
                                << "id already exists";
  }
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Created a new client "
      << "handler for "
      << (client_app_id ? "\"" + *client_app_id + "\"" : "own app")
      << " (handler id " << handler_id << ")";
}

void PcscLiteServerClientsManager::DeleteHandler(int64_t handler_id) {
  const auto handler_map_iter = handler_map_.find(handler_id);
  if (handler_map_iter == handler_map_.end()) {
    GOOGLE_SMART_CARD_LOG_FATAL << kLoggingPrefix << "Trying to delete "
                                << "a non-existing client handler with id "
                                << handler_id;
  }
  const Handler* const handler = handler_map_iter->second.get();
  const optional<std::string> client_app_id = handler->client_app_id();
  handler_map_.erase(handler_map_iter);
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Deleted client handler "
      << "for " << (client_app_id ? "\"" + *client_app_id + "\"" : "own app")
      << " (handler id was " << handler_id << ")";
}

void PcscLiteServerClientsManager::DeleteAllHandlers() {
  if (!handler_map_.empty()) {
    handler_map_.clear();
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Deleted all "
                                << handler_map_.size() << " client handlers";
  }
}

}  // namespace google_smart_card
