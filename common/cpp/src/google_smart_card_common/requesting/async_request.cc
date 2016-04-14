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

#include <google_smart_card_common/requesting/async_request.h>

#include <utility>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

AsyncRequestState::AsyncRequestState(AsyncRequestCallback callback)
    : callback_(callback) {}

bool AsyncRequestState::SetResult(GenericRequestResult request_result) {
  if (!is_callback_called_.test_and_set()) {
    callback_(std::move(request_result));
    return true;
  }
  return false;
}

AsyncRequest::AsyncRequest() {}

AsyncRequest::AsyncRequest(std::shared_ptr<AsyncRequestState> state)
    : state_(state) {
  GOOGLE_SMART_CARD_CHECK(state_);
}

bool AsyncRequest::Cancel() {
  const std::shared_ptr<AsyncRequestState> state = std::atomic_load(&state_);
  GOOGLE_SMART_CARD_CHECK(state);
  return state->SetResult(GenericRequestResult::CreateCanceled());
}

AsyncRequest& AsyncRequest::operator=(const AsyncRequest& other) {
  std::atomic_store(&state_, other.state_);
  return *this;
}

}  // namespace google_smart_card
