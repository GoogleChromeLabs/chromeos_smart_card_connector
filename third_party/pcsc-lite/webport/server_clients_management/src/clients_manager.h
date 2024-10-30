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

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/messaging/typed_message_listener.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/requesting/js_request_receiver.h"
#include "common/cpp/src/public/requesting/request_handler.h"
#include "common/cpp/src/public/requesting/request_receiver.h"
#include "common/cpp/src/public/value.h"
#include "third_party/pcsc-lite/webport/server_clients_management/src/admin_policy_getter.h"
#include "third_party/pcsc-lite/webport/server_clients_management/src/client_request_processor.h"

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
// 1. The manager receives a special "create client handler" message with the
//    supplied unique client handler id.
//    As a result, the manager creates an internal class
//    PcscFunctionCallRequestHandler that holds an instance of
//    PcscLiteClientRequestProcessor (that is actually an object that performs
//    client requests and keeps the set of its handles and checks it) and an
//    instance of JsRequestReceiver (that subscribes for receiving client
//    request messages and passing them to the Client instance, which redirects
//    them to the PcscLiteClientRequestProcessor instance).
// 2. The manager receives a special "remove client" message with the client id.
//    As a result, the manager removes the corresponding instance of the
//    PcscLiteServerClientsManager::Client class, which, in turn, shuts down the
//    JsRequestReceiver owned by it - so this ensures that new requests for
//    this client won't be received, and the responses for the currently
//    processed requests from this client will be discarded.
//    Note that removing a client does not imply an immediate destruction of the
//    corresponding PcscLiteClientRequestProcessor instance: there may be long-
//    running requests currently being processed by this class. (Thanks to the
//    refcounting-based storage of the PcscLiteClientRequestProcessor class,
//    its instance gets destroyed after the last request is finished.)
//
// Note that this class does _not_ perform permission checking regarding
// whether a client is allowed to issue PC/SC function calls. This should have
// already been done on the JavaScript side before sending client handler
// creation messages.
//
// FIXME(emaxx): Add assertions that the class methods are always executed on
// the same thread.
class PcscLiteServerClientsManager final {
 public:
  PcscLiteServerClientsManager(GlobalContext* global_context,
                               TypedMessageRouter* typed_message_router,
                               AdminPolicyGetter* admin_policy_getter);

  PcscLiteServerClientsManager(const PcscLiteServerClientsManager&) = delete;
  PcscLiteServerClientsManager& operator=(const PcscLiteServerClientsManager&) =
      delete;

  ~PcscLiteServerClientsManager();

  void ShutDown();

 private:
  // Message listener for the client handler creation messages received from the
  // JavaScript side. This class acts like a proxy, delegating the actual
  // handling of the message to the associated PcscLiteServerClientsManager
  // instance.
  class CreateHandlerMessageListener final : public TypedMessageListener {
   public:
    explicit CreateHandlerMessageListener(
        PcscLiteServerClientsManager* clients_manager);
    CreateHandlerMessageListener(const CreateHandlerMessageListener&) = delete;

    // TypedMessageListener:
    std::string GetListenedMessageType() const override;
    bool OnTypedMessageReceived(Value data) override;

   private:
    PcscLiteServerClientsManager* clients_manager_;
  };

  // Message listener for the client handler deletion messages received from the
  // JavaScript side. This class acts like a proxy, delegating the actual
  // handling of the message to the associated PcscLiteServerClientsManager
  // instance.
  class DeleteHandlerMessageListener final : public TypedMessageListener {
   public:
    explicit DeleteHandlerMessageListener(
        PcscLiteServerClientsManager* clients_manager);
    DeleteHandlerMessageListener(const DeleteHandlerMessageListener&) = delete;

    // TypedMessageListener:
    std::string GetListenedMessageType() const override;
    bool OnTypedMessageReceived(Value data) override;

   private:
    PcscLiteServerClientsManager* clients_manager_;
  };

  // Request handler that handles the PC/SC function call requests received from
  // a client (and delivered here by the JavaScript side).
  class Handler final : public RequestHandler {
   public:
    Handler(int64_t handler_id,
            const std::string& client_name_for_log,
            GlobalContext* global_context,
            TypedMessageRouter* typed_message_router,
            AdminPolicyGetter* admin_policy_getter);
    Handler(const Handler&) = delete;

    ~Handler() override;

    const std::string& client_name_for_log() const {
      return client_name_for_log_;
    }

    // RequestHandler:
    void HandleRequest(
        Value payload,
        RequestReceiver::ResultCallback result_callback) override;

   private:
    const int64_t handler_id_;
    const std::string client_name_for_log_;
    std::shared_ptr<PcscLiteClientRequestProcessor> request_processor_;
    std::shared_ptr<JsRequestReceiver> request_receiver_;
  };

  void CreateHandler(int64_t handler_id,
                     const std::string& client_name_for_log);
  void DeleteHandler(int64_t client_handler_id);
  void DeleteAllHandlers();

  GlobalContext* const global_context_;
  TypedMessageRouter* typed_message_router_;
  AdminPolicyGetter* const admin_policy_getter_;
  CreateHandlerMessageListener create_handler_message_listener_;
  DeleteHandlerMessageListener delete_handler_message_listener_;
  std::unordered_map<int64_t, std::unique_ptr<Handler>> handler_map_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_CLIENTS_MANAGER_H_
