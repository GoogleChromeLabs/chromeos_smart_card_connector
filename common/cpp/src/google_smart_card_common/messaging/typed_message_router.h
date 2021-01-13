// Copyright 2016 Google Inc. All Rights Reserved.
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

#ifndef GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_ROUTER_H_
#define GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_ROUTER_H_

#include <mutex>
#include <string>
#include <unordered_map>

#include <google_smart_card_common/messaging/message_listener.h>
#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// The router that handles incoming messages by routing them to the correct
// listener.
//
// The routing is based on the message type key (which has to be a string): the
// listener whose GetListenedMessageType method returns the same string will
// receive the message.
// If there was no corresponding listener found, or if the message has a wrong
// format, then the message is left unhandled (and false is returned from the
// OnMessageReceived method).
//
// The class is generally thread-safe. Note that, however, it's the consumer's
// responsibility to deal with the situations when a listener is removed at the
// same time when a message routed to it is being processed (in that case the
// listener may receive the message even after the RemoveRoute method was
// called).
class TypedMessageRouter final : public MessageListener {
 public:
  TypedMessageRouter();
  TypedMessageRouter(const TypedMessageRouter&) = delete;
  TypedMessageRouter& operator=(const TypedMessageRouter&) = delete;
  ~TypedMessageRouter() override;

  // Adds a new listener, which will handle all messages having the type equal
  // to the GetListenedMessageType return value.
  //
  // Asserts that no listener has already been registered for the same type.
  void AddRoute(TypedMessageListener* listener);

  // Removes a previously added listener.
  //
  // Asserts that the listener was actually added.
  void RemoveRoute(TypedMessageListener* listener);

  // MessageListener:
  bool OnMessageReceived(Value message,
                         std::string* error_message = nullptr) override;

 private:
  TypedMessageListener* FindListenerByType(
      const std::string& message_type) const;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, TypedMessageListener*> route_map_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_ROUTER_H_
