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

#include <google_smart_card_common/requesting/remote_call_adaptor.h>

#include <utility>

#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

RemoteCallAdaptor::RemoteCallAdaptor(Requester* requester)
    : requester_(requester) {
  GOOGLE_SMART_CARD_CHECK(requester_);
}

RemoteCallAdaptor::~RemoteCallAdaptor() = default;

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
