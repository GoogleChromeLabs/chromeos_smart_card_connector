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

#include "common/cpp/src/public/requesting/js_request_receiver.h"

#include <utility>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/requesting/requester_message.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"

namespace google_smart_card {

JsRequestReceiver::JsRequestReceiver(const std::string& name,
                                     RequestHandler* request_handler,
                                     GlobalContext* global_context,
                                     TypedMessageRouter* typed_message_router)
    : RequestReceiver(name, request_handler),
      global_context_(global_context),
      typed_message_router_(typed_message_router) {
  GOOGLE_SMART_CARD_CHECK(global_context_);
  GOOGLE_SMART_CARD_CHECK(typed_message_router_);
  typed_message_router->AddRoute(this);
}

JsRequestReceiver::~JsRequestReceiver() {
  ShutDown();
}

void JsRequestReceiver::ShutDown() {
  TypedMessageRouter* const typed_message_router =
      typed_message_router_.exchange(nullptr);
  if (typed_message_router)
    typed_message_router->RemoveRoute(this);
}

void JsRequestReceiver::PostResult(RequestId request_id,
                                   GenericRequestResult request_result) {
  TypedMessage message;
  message.type = GetResponseMessageType(name());
  message.data =
      ConvertToValueOrDie(ResponseMessageData::CreateFromRequestResult(
          request_id, std::move(request_result)));
  Value message_value = ConvertToValueOrDie(std::move(message));
  global_context_->PostMessageToJs(std::move(message_value));
}

std::string JsRequestReceiver::GetListenedMessageType() const {
  return GetRequestMessageType(name());
}

bool JsRequestReceiver::OnTypedMessageReceived(Value data) {
  RequestMessageData message_data =
      ConvertFromValueOrDie<RequestMessageData>(std::move(data));
  HandleRequest(message_data.request_id, std::move(message_data.payload));
  return true;
}

}  // namespace google_smart_card
