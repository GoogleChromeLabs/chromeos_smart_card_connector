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

#include <google_smart_card_common/requesting/js_requester.h>

#include <utility>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

namespace google_smart_card {

namespace {

constexpr char kRequesterIsDetachedErrorMessage[] =
    "The requester is in the detached state";

}  // namespace

JsRequester::PpDelegateImpl::PpDelegateImpl(pp::Instance* pp_instance,
                                            pp::Core* pp_core)
    : pp_instance_(pp_instance), pp_core_(pp_core) {
  GOOGLE_SMART_CARD_CHECK(pp_instance);
  GOOGLE_SMART_CARD_CHECK(pp_core);
}

JsRequester::PpDelegateImpl::~PpDelegateImpl() = default;

void JsRequester::PpDelegateImpl::PostMessage(const pp::Var& message) {
  pp_instance_->PostMessage(message);
}

bool JsRequester::PpDelegateImpl::IsMainThread() {
  return pp_core_->IsMainThread();
}

JsRequester::JsRequester(const std::string& name,
                         TypedMessageRouter* typed_message_router,
                         std::unique_ptr<PpDelegate> pp_delegate)
    : Requester(name),
      typed_message_router_(typed_message_router),
      pp_delegate_(std::move(pp_delegate)) {
  GOOGLE_SMART_CARD_CHECK(typed_message_router);
  GOOGLE_SMART_CARD_CHECK(pp_delegate_);
  typed_message_router->AddRoute(this);
}

JsRequester::~JsRequester() {
  Detach();
}

void JsRequester::Detach() {
  TypedMessageRouter* const typed_message_router =
      typed_message_router_.exchange(nullptr);
  if (typed_message_router)
    typed_message_router->RemoveRoute(this);

  pp_delegate_.Reset();

  Requester::Detach();
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
  // TODO(#185): Directly send `Value` instead of `pp::Var`.
  const pp::Var typed_message_pp_var = ConvertValueToPpVar(typed_message_value);

  if (!PostPpMessage(typed_message_pp_var)) {
    SetAsyncRequestResult(request_id, GenericRequestResult::CreateFailed(
                                          kRequesterIsDetachedErrorMessage));
  }
}

GenericRequestResult JsRequester::PerformSyncRequest(Value payload) {
  GOOGLE_SMART_CARD_CHECK(!IsMainPpThread());
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

bool JsRequester::PostPpMessage(const pp::Var& message) {
  const ThreadSafeUniquePtr<PpDelegate>::Locked pp_delegate =
      pp_delegate_.Lock();
  if (!pp_delegate)
    return false;
  pp_delegate->PostMessage(message);
  return true;
}

bool JsRequester::IsMainPpThread() const {
  const ThreadSafeUniquePtr<PpDelegate>::Locked pp_delegate =
      pp_delegate_.Lock();
  if (!pp_delegate) {
    // Note that returning false in this case may actually be wrong, but this
    // method is used only before starting a new request - that will fail
    // immediately in this case anyway.
    return false;
  }
  return pp_delegate->IsMainThread();
}

}  // namespace google_smart_card
