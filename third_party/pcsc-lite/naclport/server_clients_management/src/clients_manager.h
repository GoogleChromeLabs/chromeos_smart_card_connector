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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENTS_MANAGER_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENTS_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/js_request_receiver.h>
#include <google_smart_card_common/requesting/request_handler.h>
#include <google_smart_card_common/requesting/request_receiver.h>

#include "client_id.h"
#include "client_request_processor.h"

namespace google_smart_card {

// This class manages the external PC/SC-Lite clients and runs PC/SC-Lite API
// requests received from them.
//
// This class is an important piece for providing privacy and security of the
// PC/SC-Lite NaCl port: it keeps all clients isolated from each other. This
// class is preventing one client from using, accessing or interfering with
// handles or requests of any other client.
//
// The workflow of the client manager object is the following:
// 1. The manager receives a special "add client" message with the supplied
//    unique client id.
//    As a result, the manager creates an internal class
//    PcscLiteServerClientsManager::Client that holds an instance of
//    PcscLiteClientRequestProcessor (that is actually an object that performs
//    client requests and keeps the set of its handles and checks it) and an
//    instance of JsRequestReceiver (that subscribes for receiving client
//    request messages and passing them to the Client instance, which redirects
//    them to the PcscLiteClientRequestProcessor instance).
// 2. The manager receives a special "remove client" message with the client id.
//    As a result, the manager removes the corresponding instance of the
//    PcscLiteServerClientsManager::Client class, which, in turn, detaches the
//    JsRequestReceiver owned by it - so this ensures that new requests for
//    this client won't be received, and the responses for the currently
//    processed requests from this client will be discarded.
//    Note that removing a client does not imply an immediate destruction of the
//    corresponding PcscLiteClientRequestProcessor instance: there may be long-
//    running requests currently being processed by this class. (Thanks to the
//    refcounting-based storage of the PcscLiteClientRequestProcessor class,
//    its instance gets destroyed after the last request is finished.)
//
// FIXME(emaxx): Add assertions that the class methods are always executed on
// the same thread.
class PcscLiteServerClientsManager final {
 public:
  PcscLiteServerClientsManager(
      pp::Instance* pp_instance, TypedMessageRouter* typed_message_router);

  ~PcscLiteServerClientsManager();

  void Detach();

 private:
  class AddClientMessageListener final : public TypedMessageListener {
   public:
    explicit AddClientMessageListener(
        PcscLiteServerClientsManager* clients_manager);
    std::string GetListenedMessageType() const override;
    bool OnTypedMessageReceived(const pp::Var& data) override;

   private:
    PcscLiteServerClientsManager* clients_manager_;
  };

  class RemoveClientMessageListener final : public TypedMessageListener {
   public:
    explicit RemoveClientMessageListener(
        PcscLiteServerClientsManager* clients_manager);
    std::string GetListenedMessageType() const override;
    bool OnTypedMessageReceived(const pp::Var& data) override;

   private:
    PcscLiteServerClientsManager* clients_manager_;
  };

  class Client final : public RequestHandler {
   public:
    Client(
        PcscLiteServerClientId client_id,
        pp::Instance* pp_instance,
        TypedMessageRouter* typed_message_router);

    ~Client() override;

    void HandleRequest(
        const pp::Var& payload,
        RequestReceiver::ResultCallback result_callback) override;

   private:
    const PcscLiteServerClientId client_id_;
    std::shared_ptr<PcscLiteClientRequestProcessor> request_processor_;
    std::shared_ptr<JsRequestReceiver> request_receiver_;
  };

  void AddNewClient(PcscLiteServerClientId client_id);
  void RemoveClient(PcscLiteServerClientId client_id);
  void RemoveAllClients();

  pp::Instance* pp_instance_;
  TypedMessageRouter* typed_message_router_;
  AddClientMessageListener add_client_message_listener_;
  RemoveClientMessageListener remove_client_message_listener_;
  std::unordered_map<PcscLiteServerClientId, std::unique_ptr<Client>>
  client_map_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENTS_MANAGER_H_
