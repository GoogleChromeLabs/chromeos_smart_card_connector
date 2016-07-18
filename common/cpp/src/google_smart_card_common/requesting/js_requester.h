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

#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/requester.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/thread_safe_unique_ptr.h>

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
  // Delegate that is used for interactions with the Pepper APIs that are tied
  // to Pepper modules and instances.
  //
  // The main reason for existence of this class is testing purposes.
  class PpDelegate {
   public:
    virtual ~PpDelegate() = default;

    virtual void PostMessage(const pp::Var& message) = 0;
    virtual bool IsMainThread() = 0;
  };

  // Implementation of the delegate that talks to the real Pepper core and
  // instance objects.
  class PpDelegateImpl final : public PpDelegate {
   public:
    PpDelegateImpl(pp::Instance* pp_instance, pp::Core* pp_core);

    void PostMessage(const pp::Var& message) override;
    bool IsMainThread() override;

   private:
    pp::Instance* const pp_instance_;
    pp::Core* const pp_core_;
  };

  // Creates a new requester.
  //
  // Adds a new route into the passed TypedMessageRouter for receiving the
  // responses messages.
  //
  // Note that the passed TypedMessageRouter is allowed to be destroyed earlier
  // than the JsRequester object - but the Detach() method must be called before
  // destroying it.
  JsRequester(
      const std::string& name,
      TypedMessageRouter* typed_message_router,
      std::unique_ptr<PpDelegate> pp_delegate);

  ~JsRequester() override;

  // Requester implementation
  void Detach() override;
  void StartAsyncRequest(
      const pp::Var& payload,
      GenericAsyncRequestCallback callback,
      GenericAsyncRequest* async_request) override;
  // Requester implementation override
  //
  // Note that it is asserted that this method is called not from the main
  // Pepper thread (as in that case waiting would block the message loop and
  // result in deadlock).
  GenericRequestResult PerformSyncRequest(const pp::Var& payload) override;

 private:
  // TypedMessageListener implementation
  std::string GetListenedMessageType() const override;
  bool OnTypedMessageReceived(const pp::Var& data) override;

  bool PostPpMessage(const pp::Var& message);
  bool IsMainPpThread() const;

  std::atomic<TypedMessageRouter*> typed_message_router_;
  ThreadSafeUniquePtr<PpDelegate> pp_delegate_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_JS_REQUESTER_H_
