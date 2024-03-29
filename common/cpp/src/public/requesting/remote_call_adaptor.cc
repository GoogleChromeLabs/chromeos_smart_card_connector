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

#include "common/cpp/src/public/requesting/remote_call_adaptor.h"

#include <utility>

#include "common/cpp/src/public/requesting/remote_call_async_request.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/value_conversion.h"

namespace google_smart_card {

RemoteCallAdaptor::RemoteCallAdaptor(Requester* requester)
    : requester_(requester) {
  GOOGLE_SMART_CARD_CHECK(requester_);
}

RemoteCallAdaptor::~RemoteCallAdaptor() = default;

void RemoteCallAdaptor::AsyncCall(
    RemoteCallAsyncRequest remote_call_async_request) {
  StartAsyncRequest(std::move(remote_call_async_request.request_payload),
                    remote_call_async_request.callback);
}

GenericRequestResult RemoteCallAdaptor::PerformSyncRequest(
    RemoteCallRequestPayload payload) {
  return requester_->PerformSyncRequest(
      ConvertToValueOrDie(std::move(payload)));
}

GenericAsyncRequest RemoteCallAdaptor::StartAsyncRequest(
    RemoteCallRequestPayload payload,
    GenericAsyncRequestCallback callback) {
  return requester_->StartAsyncRequest(ConvertToValueOrDie(std::move(payload)),
                                       callback);
}

}  // namespace google_smart_card
