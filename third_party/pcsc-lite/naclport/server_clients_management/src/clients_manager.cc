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

#include <sstream>
#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/struct_converter.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/unique_ptr_utils.h>

#include "client_request_processor.h"

const char kAddClientMessageType[] = "pcsc_lite_add_client";
const char kRemoveClientMessageType[] = "pcsc_lite_remove_client";
const char kClientRequesterNamePrefix[] = "pcsc_lite_client_";
const char kClientRequesterNameSuffix[] = "_call_function";
const char kLoggingPrefix[] = "[PC/SC-Lite Client Manager] ";

namespace google_smart_card {

namespace {

struct AddClientRequest {
  PcscLiteServerClientId client_id;
};

struct RemoveClientRequest {
  PcscLiteServerClientId client_id;
};

}  // namespace

template <>
constexpr const char* StructConverter<AddClientRequest>::GetStructTypeName() {
  return "AddClientRequest";
}

template <>
template <typename Callback>
void StructConverter<AddClientRequest>::VisitFields(
    const AddClientRequest& value, Callback callback) {
  callback(&value.client_id, "client_id");
}

template <>
constexpr const char*
StructConverter<RemoveClientRequest>::GetStructTypeName() {
  return "RemoveClientRequest";
}

template <>
template <typename Callback>
void StructConverter<RemoveClientRequest>::VisitFields(
    const RemoveClientRequest& value, Callback callback) {
  callback(&value.client_id, "client_id");
}

namespace {

std::string GetClientRequesterName(PcscLiteServerClientId client_id) {
  std::ostringstream stream;
  stream << kClientRequesterNamePrefix << client_id <<
      kClientRequesterNameSuffix;
  return stream.str();
}

}  // namespace

PcscLiteServerClientsManager::PcscLiteServerClientsManager(
    pp::Instance* pp_instance, TypedMessageRouter* typed_message_router)
    : pp_instance_(pp_instance),
      typed_message_router_(typed_message_router),
      add_client_message_listener_(this),
      remove_client_message_listener_(this) {
  GOOGLE_SMART_CARD_CHECK(pp_instance);
  GOOGLE_SMART_CARD_CHECK(typed_message_router);

  typed_message_router_->AddRoute(&add_client_message_listener_);
  typed_message_router_->AddRoute(&remove_client_message_listener_);
}

PcscLiteServerClientsManager::~PcscLiteServerClientsManager() {
  Detach();
}

void PcscLiteServerClientsManager::Detach() {
  typed_message_router_->RemoveRoute(&add_client_message_listener_);
  typed_message_router_->RemoveRoute(&remove_client_message_listener_);
  RemoveAllClients();
}

PcscLiteServerClientsManager::AddClientMessageListener::AddClientMessageListener(
    PcscLiteServerClientsManager* clients_manager)
    : clients_manager_(clients_manager) {}

std::string
PcscLiteServerClientsManager::AddClientMessageListener::GetListenedMessageType()
const {
  return kAddClientMessageType;
}

bool
PcscLiteServerClientsManager::AddClientMessageListener::OnTypedMessageReceived(
    const pp::Var& data) {
  AddClientRequest add_client_request;
  std::string error_message;
  if (!StructConverter<AddClientRequest>::ConvertFromVar(
           data, &add_client_request, &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << kLoggingPrefix << "Failed to parse " <<
        "client add request: " << error_message;
  }

  clients_manager_->AddNewClient(add_client_request.client_id);
  return true;
}

PcscLiteServerClientsManager::RemoveClientMessageListener
::RemoveClientMessageListener(
    PcscLiteServerClientsManager* clients_manager)
    : clients_manager_(clients_manager) {}

std::string PcscLiteServerClientsManager::RemoveClientMessageListener
::GetListenedMessageType() const {
  return kRemoveClientMessageType;
}

bool PcscLiteServerClientsManager::RemoveClientMessageListener
::OnTypedMessageReceived(const pp::Var& data) {
  RemoveClientRequest remove_client_request;
  std::string error_message;
  if (!StructConverter<RemoveClientRequest>::ConvertFromVar(
           data, &remove_client_request, &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << kLoggingPrefix << "Failed to parse " <<
        "client remove request: " << error_message;
  }

  clients_manager_->RemoveClient(remove_client_request.client_id);
  return true;
}

PcscLiteServerClientsManager::Client::Client(
    PcscLiteServerClientId client_id,
    pp::Instance* pp_instance,
    TypedMessageRouter* typed_message_router)
    : client_id_(client_id),
      request_processor_(new PcscLiteClientRequestProcessor(client_id)),
      request_receiver_(new JsRequestReceiver(
          GetClientRequesterName(client_id),
          this,
          typed_message_router,
          MakeUnique<JsRequestReceiver::PpDelegateImpl>(pp_instance))) {}

PcscLiteServerClientsManager::Client::~Client() {
  request_receiver_->Detach();
}

void PcscLiteServerClientsManager::Client::HandleRequest(
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

void PcscLiteServerClientsManager::AddNewClient(
    PcscLiteServerClientId client_id) {
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Adding a new client (id " <<
      client_id << ")";

  std::unique_ptr<Client> client(new Client(
      client_id, pp_instance_, typed_message_router_));
  if (!client_map_.emplace(client_id, std::move(client)).second) {
    GOOGLE_SMART_CARD_LOG_FATAL << kLoggingPrefix << "Failed to add a new " <<
        "client with id " << client_id << ": client with this id already " <<
        "exists";
  }
}

void PcscLiteServerClientsManager::RemoveClient(
    PcscLiteServerClientId client_id) {
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Removing the client (id " <<
      client_id << ")";

  const auto client_map_iter = client_map_.find(client_id);
  if (client_map_iter == client_map_.end()) {
    GOOGLE_SMART_CARD_LOG_FATAL << kLoggingPrefix << "Trying to remove " <<
        "a non-existing client with id " << client_id;
  }
  client_map_.erase(client_map_iter);
}

void PcscLiteServerClientsManager::RemoveAllClients() {
  if (!client_map_.empty()) {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Removing all " <<
        client_map_.size() << " clients";
    client_map_.clear();
  }
}

}  // namespace google_smart_card
