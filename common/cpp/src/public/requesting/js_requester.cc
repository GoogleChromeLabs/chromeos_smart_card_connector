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

#include "common/cpp/src/public/requesting/js_requester.h"

#include <utility>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/requesting/request_id.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"

namespace google_smart_card {

JsRequester::JsRequester(const std::string& name,
                         GlobalContext* global_context,
                         TypedMessageRouter* typed_message_router)
    : Requester(name),
      global_context_(global_context),
      typed_message_router_(typed_message_router) {
  GOOGLE_SMART_CARD_CHECK(global_context_);
  GOOGLE_SMART_CARD_CHECK(typed_message_router_);
  typed_message_router->AddRoute(this);
}

JsRequester::~JsRequester() {
  ShutDown();
}

void JsRequester::ShutDown() {
  TypedMessageRouter* const typed_message_router =
      typed_message_router_.exchange(nullptr);
  if (typed_message_router)
    typed_message_router->RemoveRoute(this);

  Requester::ShutDown();
}

void JsRequester::StartAsyncRequest(Value payload,
                                    GenericAsyncRequestCallback callback,
                                    GenericAsyncRequest* async_request) {
  RequestId request_id;
  *async_request = CreateAsyncRequest(callback, &request_id);

  RequestMessageData message_data;
  message_data.request_id = request_id;
  message_data.payload = std::move(payload);
  TypedMessage typed_message;
  typed_message.type = GetRequestMessageType(name());
  typed_message.data = ConvertToValueOrDie(std::move(message_data));
  Value typed_message_value = ConvertToValueOrDie(std::move(typed_message));

  // Note: This message won't arrive to the JS side in case the shutdown process
  // started; it's not a concern, since it means that new requests just won't
  // complete.
  global_context_->PostMessageToJs(std::move(typed_message_value));
}

GenericRequestResult JsRequester::PerformSyncRequest(Value payload) {
  // Synchronous requests aren't allowed on the main event loop thread, since
  // it'll be deadlocked otherwise (as response messages won't arrive).
  GOOGLE_SMART_CARD_CHECK(!global_context_->IsMainEventLoopThread());

  return Requester::PerformSyncRequest(std::move(payload));
}

std::string JsRequester::GetListenedMessageType() const {
  return GetResponseMessageType(name());
}

bool JsRequester::OnTypedMessageReceived(Value data) {
  ResponseMessageData message_data =
      ConvertFromValueOrDie<ResponseMessageData>(std::move(data));
  GenericRequestResult request_result;
  GOOGLE_SMART_CARD_CHECK(message_data.ExtractRequestResult(&request_result));
  GOOGLE_SMART_CARD_CHECK(SetAsyncRequestResult(message_data.request_id,
                                                std::move(request_result)));
  return true;
}

}  // namespace google_smart_card
