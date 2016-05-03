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

#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/requesting/js_request_receiver.h>
#include <google_smart_card_common/requesting/requester_message.h>

namespace google_smart_card {

JsRequestReceiver::PpDelegateImpl::PpDelegateImpl(pp::Instance* pp_instance)
    : pp_instance_(pp_instance) {
  GOOGLE_SMART_CARD_CHECK(pp_instance);
}

void JsRequestReceiver::PpDelegateImpl::PostMessage(const pp::Var& message) {
  pp_instance_->PostMessage(message);
}

JsRequestReceiver::JsRequestReceiver(
    const std::string& name,
    RequestHandler* request_handler,
    TypedMessageRouter* typed_message_router,
    std::unique_ptr<PpDelegate> pp_delegate)
    : RequestReceiver(name, request_handler),
      typed_message_router_(typed_message_router),
      pp_delegate_(std::move(pp_delegate)) {
  GOOGLE_SMART_CARD_CHECK(typed_message_router);
  GOOGLE_SMART_CARD_CHECK(pp_delegate_);
  typed_message_router->AddRoute(this);
}

JsRequestReceiver::~JsRequestReceiver() {
  Detach();
}

void JsRequestReceiver::Detach() {
  TypedMessageRouter* const typed_message_router =
      typed_message_router_.exchange(nullptr);
  if (typed_message_router)
    typed_message_router->RemoveRoute(this);

  pp_delegate_.Reset();
}

void JsRequestReceiver::PostResult(
    RequestId request_id, GenericRequestResult request_result) {
  PostResultMessage(MakeTypedMessage(
      GetResponseMessageType(name()),
      MakeResponseMessageData(request_id, std::move(request_result))));
}

std::string JsRequestReceiver::GetListenedMessageType() const {
  return GetRequestMessageType(name());
}

bool JsRequestReceiver::OnTypedMessageReceived(const pp::Var& data) {
  RequestId request_id;
  pp::Var payload;
  GOOGLE_SMART_CARD_CHECK(ParseRequestMessageData(data, &request_id, &payload));
  HandleRequest(request_id, payload);
  return true;
}

void JsRequestReceiver::PostResultMessage(const pp::Var& message) {
  const ThreadSafeUniquePtr<PpDelegate>::Locked pp_delegate =
      pp_delegate_.Lock();
  if (!pp_delegate) {
    // TODO(emaxx): Add some logging?..
    return;
  }
  pp_delegate->PostMessage(message);
}

}  // namespace google_smart_card
