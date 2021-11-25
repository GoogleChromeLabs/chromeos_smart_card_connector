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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUEST_RECEIVER_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUEST_RECEIVER_H_

#include <atomic>
#include <memory>
#include <string>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/request_handler.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_receiver.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Request receiver that receives the requests from JavaScript and sends the
// responses messages back to it.
//
// The name is used for listening for the messages sent by the corresponding
// requester (see GetRequestMessageType() and GetResponseMessageType() functions
// in common/requesting/requester_message.h header).
//
// Outgoing messages (with requests) are sent using the PpDelegate instance.
// Incoming messages (with responses) are received through adding a new route
// into the specified TypedMessageRouter instance.
class JsRequestReceiver final : public RequestReceiver,
                                public TypedMessageListener {
 public:
  // Creates a new request receiver.
  //
  // Adds a new route into the passed TypedMessageRouter for receiving the
  // requests messages.
  JsRequestReceiver(const std::string& name,
                    RequestHandler* request_handler,
                    GlobalContext* global_context,
                    TypedMessageRouter* typed_message_router);

  JsRequestReceiver(const JsRequestReceiver&) = delete;
  JsRequestReceiver& operator=(const JsRequestReceiver&) = delete;

  ~JsRequestReceiver() override;

  // Stop sending request responses and prevent receiving of new requests (as
  // the corresponding route gets removed from the associated TypedMessageRouter
  // object).
  //
  // This function is safe to be called from any thread.
  void ShutDown();

 private:
  // RequestReceiver:
  void PostResult(RequestId request_id,
                  GenericRequestResult request_result) override;

  // TypedMessageListener:
  std::string GetListenedMessageType() const override;
  bool OnTypedMessageReceived(Value data) override;

  GlobalContext* const global_context_;
  std::atomic<TypedMessageRouter*> typed_message_router_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUEST_RECEIVER_H_
