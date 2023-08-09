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

#include "third_party/pcsc-lite/naclport/server_clients_management/src/clients_manager.h"

#include <inttypes.h>

#include <sstream>
#include <string>
#include <utility>

#include "common/cpp/src/public/formatting.h"
#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "third_party/pcsc-lite/naclport/server_clients_management/src/admin_policy_getter.h"
#include "third_party/pcsc-lite/naclport/server_clients_management/src/client_request_processor.h"

namespace google_smart_card {

namespace {

constexpr char kCreateHandlerMessageType[] = "pcsc_lite_create_client_handler";
constexpr char kDeleteHandlerMessageType[] = "pcsc_lite_delete_client_handler";
constexpr char kLoggingPrefix[] = "[PC/SC-Lite clients manager] ";

// The structure represents the message data contents for the client handler
// creation message.
struct CreateHandlerMessageData {
  int64_t handler_id;
  std::string client_name_for_log;
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
  // Note: Strings passed to WithField() below must match the keys in
  // //third_party/pcsc-lite/naclport/server_clients_management/src/client-handler.js.
  return Describe("CreateHandlerMessageData")
      .WithField(&CreateHandlerMessageData::handler_id, "handler_id")
      .WithField(&CreateHandlerMessageData::client_name_for_log,
                 "client_name_for_log");
}

template <>
StructValueDescriptor<DeleteHandlerMessageData>::Description
StructValueDescriptor<DeleteHandlerMessageData>::GetDescription() {
  // Note: Strings passed to WithField() below must match the keys in
  // //third_party/pcsc-lite/naclport/server_clients_management/src/client-handler.js.
  return Describe("DeleteHandlerMessageData")
      .WithField(&DeleteHandlerMessageData::handler_id, "handler_id");
}

PcscLiteServerClientsManager::PcscLiteServerClientsManager(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router,
    AdminPolicyGetter* admin_policy_getter)
    : global_context_(global_context),
      typed_message_router_(typed_message_router),
      admin_policy_getter_(admin_policy_getter),
      create_handler_message_listener_(this),
      delete_handler_message_listener_(this) {
  GOOGLE_SMART_CARD_CHECK(global_context_);
  GOOGLE_SMART_CARD_CHECK(typed_message_router_);
  GOOGLE_SMART_CARD_CHECK(admin_policy_getter_);

  typed_message_router_->AddRoute(&create_handler_message_listener_);
  typed_message_router_->AddRoute(&delete_handler_message_listener_);
  typed_message_router_->AddRoute(admin_policy_getter_);
}

PcscLiteServerClientsManager::~PcscLiteServerClientsManager() {
  ShutDown();
}

void PcscLiteServerClientsManager::ShutDown() {
  if (!typed_message_router_)
    return;
  typed_message_router_->RemoveRoute(&create_handler_message_listener_);
  typed_message_router_->RemoveRoute(&delete_handler_message_listener_);
  typed_message_router_->RemoveRoute(admin_policy_getter_);
  admin_policy_getter_->ShutDown();
  DeleteAllHandlers();
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
  const CreateHandlerMessageData message_data =
      ConvertFromValueOrDie<CreateHandlerMessageData>(std::move(data));
  clients_manager_->CreateHandler(message_data.handler_id,
                                  message_data.client_name_for_log);
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
  const DeleteHandlerMessageData message_data =
      ConvertFromValueOrDie<DeleteHandlerMessageData>(std::move(data));
  clients_manager_->DeleteHandler(message_data.handler_id);
  return true;
}

PcscLiteServerClientsManager::Handler::Handler(
    int64_t handler_id,
    const std::string& client_name_for_log,
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router,
    AdminPolicyGetter* admin_policy_getter)
    : handler_id_(handler_id),
      client_name_for_log_(client_name_for_log),
      request_processor_(
          new PcscLiteClientRequestProcessor(handler_id,
                                             client_name_for_log_,
                                             admin_policy_getter)),
      request_receiver_(new JsRequestReceiver(
          FormatPrintfTemplate("pcsc_lite_client_handler_%" PRId64
                               "_call_function",
                               handler_id),
          this,
          global_context,
          typed_message_router)) {}

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
  request_receiver_->ShutDown();
}

void PcscLiteServerClientsManager::Handler::HandleRequest(
    Value payload,
    RequestReceiver::ResultCallback result_callback) {
  RemoteCallRequestPayload remote_call_request;
  std::string error_message;
  if (!ConvertFromValue(std::move(payload), &remote_call_request,
                        &error_message)) {
    result_callback(GenericRequestResult::CreateFailed(
        "Failed to parse remote call request payload: " + error_message));
    return;
  }

  PcscLiteClientRequestProcessor::AsyncProcessRequest(
      request_processor_, std::move(remote_call_request), result_callback);
}

void PcscLiteServerClientsManager::CreateHandler(
    int64_t handler_id,
    const std::string& client_name_for_log) {
  std::unique_ptr<Handler> handler(
      new Handler(handler_id, client_name_for_log, global_context_,
                  typed_message_router_, admin_policy_getter_));
  if (!handler_map_.emplace(handler_id, std::move(handler)).second) {
    GOOGLE_SMART_CARD_LOG_FATAL << kLoggingPrefix << "Failed to create a "
                                << "new client handler with id " << handler_id
                                << ": handler with this "
                                << "id already exists";
  }
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Created a new client "
      << "handler for "
      << (client_name_for_log.empty() ? "ourselves" : client_name_for_log)
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
  const std::string client_name_for_log = handler->client_name_for_log();
  handler_map_.erase(handler_map_iter);
  GOOGLE_SMART_CARD_LOG_DEBUG
      << kLoggingPrefix << "Deleted client handler "
      << "for "
      << (client_name_for_log.empty() ? "ourselves" : client_name_for_log)
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
