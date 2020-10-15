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

#include <google_smart_card_common/requesting/requester.h>

#include <condition_variable>
#include <mutex>
#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>

namespace google_smart_card {

namespace {

constexpr char kRequesterDestroyedErrorMessage[] =
    "The requester was destroyed";

}  // namespace

Requester::Requester(const std::string& name) : name_(name) {}

Requester::~Requester() {
  const std::vector<std::shared_ptr<GenericAsyncRequestState>> request_states =
      async_requests_storage_.PopAll();
  for (const auto& request_state : request_states) {
    request_state->SetResult(
        GenericRequestResult::CreateFailed(kRequesterDestroyedErrorMessage));
  }
}

GenericAsyncRequest Requester::StartAsyncRequest(
    const pp::Var& payload, GenericAsyncRequestCallback callback) {
  GenericAsyncRequest async_result;
  StartAsyncRequest(payload, callback, &async_result);
  return async_result;
}

GenericRequestResult Requester::PerformSyncRequest(const pp::Var& payload) {
  std::mutex mutex;
  std::condition_variable condition;
  optional<GenericRequestResult> result;

  StartAsyncRequest(payload, [&mutex, &condition,
                              &result](GenericRequestResult async_result) {
    GOOGLE_SMART_CARD_CHECK(!result);
    std::unique_lock<std::mutex> lock(mutex);
    result = std::move(async_result);
    condition.notify_one();
  });

  std::unique_lock<std::mutex> lock(mutex);
  condition.wait(lock, [&result] { return !!result; });

  GOOGLE_SMART_CARD_CHECK(result->status() != RequestResultStatus::kCanceled);
  return std::move(*result);
}

GenericAsyncRequest Requester::CreateAsyncRequest(
    const pp::Var& /*payload*/, GenericAsyncRequestCallback callback,
    RequestId* request_id) {
  // TODO(emaxx): The payload argument is ignored for now, but it can be
  // utilized for creating some more informative logging at the place where the
  // requests results are handled.

  const auto async_request_state =
      std::make_shared<GenericAsyncRequestState>(callback);
  *request_id = async_requests_storage_.Push(async_request_state);
  return GenericAsyncRequest(async_request_state);
}

bool Requester::SetAsyncRequestResult(RequestId request_id,
                                      GenericRequestResult request_result) {
  const std::shared_ptr<GenericAsyncRequestState> async_request_state =
      async_requests_storage_.Pop(request_id);
  if (!async_request_state) return false;
  async_request_state->SetResult(std::move(request_result));
  return true;
}

}  // namespace google_smart_card
