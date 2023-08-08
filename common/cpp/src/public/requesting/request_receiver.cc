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

#include "common/cpp/src/public/requesting/request_receiver.h"

#include <utility>

#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/requesting/request_handler.h"
#include "common/cpp/src/public/value.h"

namespace google_smart_card {

RequestReceiver::RequestReceiver(const std::string& name,
                                 RequestHandler* handler)
    : name_(name), handler_(handler) {
  GOOGLE_SMART_CARD_CHECK(handler_);
}

RequestReceiver::~RequestReceiver() = default;

std::string RequestReceiver::name() const {
  return name_;
}

void RequestReceiver::HandleRequest(RequestId request_id, Value payload) {
  handler_->HandleRequest(std::move(payload), MakeResultCallback(request_id));
}

RequestReceiver::ResultCallbackImpl::ResultCallbackImpl(
    RequestId request_id,
    std::weak_ptr<RequestReceiver> request_receiver)
    : request_id_(request_id), request_receiver_(request_receiver) {}

RequestReceiver::ResultCallbackImpl::~ResultCallbackImpl() = default;

void RequestReceiver::ResultCallbackImpl::operator()(
    GenericRequestResult request_result) {
  const std::shared_ptr<RequestReceiver> locked_request_receiver(
      request_receiver_.lock());
  if (locked_request_receiver)
    locked_request_receiver->PostResult(request_id_, std::move(request_result));
}

RequestReceiver::ResultCallback RequestReceiver::MakeResultCallback(
    RequestId request_id) {
  return ResultCallbackImpl(request_id,
                            std::weak_ptr<RequestReceiver>(shared_from_this()));
}

}  // namespace google_smart_card
