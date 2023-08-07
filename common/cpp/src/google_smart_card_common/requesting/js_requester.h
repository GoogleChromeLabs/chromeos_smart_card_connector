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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUESTER_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUESTER_H_

#include <atomic>
#include <memory>
#include <string>

#include "common/cpp/src/google_smart_card_common/global_context.h"
#include "common/cpp/src/google_smart_card_common/messaging/typed_message_listener.h"
#include "common/cpp/src/google_smart_card_common/messaging/typed_message_router.h"
#include "common/cpp/src/google_smart_card_common/requesting/requester.h"
#include "common/cpp/src/google_smart_card_common/requesting/requester_message.h"
#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

// Requester that sends the requests to JavaScript and listens for the response
// messages from it.
//
// The requester name is used for tagging the sent and received messages (see
// GetRequestMessageType() and GetResponseMessageType() functions in
// common/requesting/requester_message.h header).
//
// Outgoing messages are sent using the PpDelegate instance. Incoming messages
// are received through adding a new route into the specified TypedMessageRouter
// instance.
class JsRequester final : public Requester, public TypedMessageListener {
 public:
  // Creates a new requester.
  //
  // Adds a new route into the passed TypedMessageRouter for receiving the
  // responses messages.
  //
  // `global_context` - must outlive `this`.
  // Note that the passed TypedMessageRouter is allowed to be destroyed earlier
  // than the JsRequester object - but the `ShutDown()` method must be called
  // before destroying it.
  JsRequester(const std::string& name,
              GlobalContext* global_context,
              TypedMessageRouter* typed_message_router);

  JsRequester(const JsRequester&) = delete;
  JsRequester& operator=(const JsRequester&) = delete;

  ~JsRequester() override;

  // Requester implementation
  void ShutDown() override;
  void StartAsyncRequest(Value payload,
                         GenericAsyncRequestCallback callback,
                         GenericAsyncRequest* async_request) override;
  // Requester implementation override
  //
  // Note that it is asserted that this method is called not from the main
  // Pepper thread (as in that case waiting would block the message loop and
  // result in deadlock).
  GenericRequestResult PerformSyncRequest(Value payload) override;

 private:
  // TypedMessageListener implementation
  std::string GetListenedMessageType() const override;
  bool OnTypedMessageReceived(Value data) override;

  GlobalContext* const global_context_;
  std::atomic<TypedMessageRouter*> typed_message_router_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUESTER_H_
