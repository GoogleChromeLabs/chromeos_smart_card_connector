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

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/request_handler.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_receiver.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/thread_safe_unique_ptr.h>

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
  // Delegate that is used for interactions with the Pepper APIs that are tied
  // to Pepper modules and instances.
  //
  // The main reason for existence of this class is testing purposes.
  class PpDelegate {
   public:
    virtual ~PpDelegate() = default;

    virtual void PostMessage(const pp::Var& message) = 0;
  };

  // Implementation of the delegate that talks to the real Pepper instance
  // object.
  class PpDelegateImpl final : public PpDelegate {
   public:
    explicit PpDelegateImpl(pp::Instance* pp_instance);

    void PostMessage(const pp::Var& message) override;

   private:
    pp::Instance* const pp_instance_;
  };

  // Creates a new request receiver.
  //
  // Adds a new route into the passed TypedMessageRouter for receiving the
  // requests messages.
  JsRequestReceiver(
      const std::string& name,
      RequestHandler* request_handler,
      TypedMessageRouter* typed_message_router,
      std::unique_ptr<PpDelegate> pp_delegate);

  JsRequestReceiver(const JsRequestReceiver&) = delete;

  ~JsRequestReceiver() override;

  // Detaches the request receiver, which prevents it from sending request
  // responses and from receiving of new requests (as the corresponding route
  // gets removed from the associated TypedMessageRouter object).
  //
  // This function is safe to be called from any thread.
  void Detach();

 private:
  // RequestReceiver implementation
  void PostResult(
      RequestId request_id, GenericRequestResult request_result) override;

  // TypedMessageListener implementation
  std::string GetListenedMessageType() const override;
  bool OnTypedMessageReceived(const pp::Var& data) override;

  void PostResultMessage(const pp::Var& message);

  std::atomic<TypedMessageRouter*> typed_message_router_;
  ThreadSafeUniquePtr<PpDelegate> pp_delegate_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUEST_RECEIVER_H_
