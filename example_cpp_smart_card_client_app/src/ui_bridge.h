// Copyright 2020 Google Inc. All Rights Reserved.
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

// This file provides definitions that allow to send/receive requests to/from
// the UI.

#ifndef SMART_CARD_CLIENT_UI_BRIDGE_H_
#define SMART_CARD_CLIENT_UI_BRIDGE_H_

#include <atomic>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/value.h>

namespace smart_card_client {

// Handler of a UI message.
class MessageFromUiHandler {
 public:
  virtual ~MessageFromUiHandler() = default;

  virtual void HandleMessageFromUi(google_smart_card::Value message) = 0;
};

// This class provides a C++ bridge for sending/receiving messages to/from the
// UI.
class UiBridge final : public google_smart_card::TypedMessageListener {
 public:
  // Creates the bridge instance.
  //
  // On construction, registers self for receiving the messages from UI through
  // the supplied TypedMessageRouter typed_message_router instance.
  //
  // The |request_handling_mutex| parameter, when non-null, allows to avoid
  // simultaneous execution of multiple requests: each next request will be
  // executed only once the previous one finishes.
  UiBridge(google_smart_card::GlobalContext* global_context,
           google_smart_card::TypedMessageRouter* typed_message_router,
           std::shared_ptr<std::mutex> request_handling_mutex);

  UiBridge(const UiBridge&) = delete;
  UiBridge& operator=(const UiBridge&) = delete;

  ~UiBridge() override;

  void ShutDown();

  void SetHandler(std::weak_ptr<MessageFromUiHandler> handler);
  void RemoveHandler();

  // Sends a message to UI.
  //
  // Note that if the UI is currently closed, the message is silently discarded.
  void SendMessageToUi(google_smart_card::Value message);

  // google_smart_card::TypedMessageListener:
  std::string GetListenedMessageType() const override;
  bool OnTypedMessageReceived(google_smart_card::Value data) override;

 private:
  google_smart_card::GlobalContext* const global_context_;
  std::atomic<google_smart_card::TypedMessageRouter*> typed_message_router_;

  std::shared_ptr<std::mutex> request_handling_mutex_;

  std::weak_ptr<MessageFromUiHandler> message_from_ui_handler_;
};

}  // namespace smart_card_client

#endif  // SMART_CARD_CLIENT_UI_BRIDGE_H_
