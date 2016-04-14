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

#include <google_smart_card_common/requesting/async_requests_storage.h>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

AsyncRequestsStorage::AsyncRequestsStorage()
    : next_free_request_id_(0) {}

RequestId AsyncRequestsStorage::Push(
    std::shared_ptr<AsyncRequestState> async_request_state) {
  const std::unique_lock<std::mutex> lock(mutex_);

  const RequestId request_id = next_free_request_id_;
  ++next_free_request_id_;

  const bool is_new_state_added = state_map_.emplace(
      request_id, async_request_state).second;
  GOOGLE_SMART_CARD_CHECK(is_new_state_added);

  return request_id;
}

std::shared_ptr<AsyncRequestState> AsyncRequestsStorage::Pop(
    RequestId request_id) {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto state_map_iter = state_map_.find(request_id);
  if (state_map_iter == state_map_.end())
    return nullptr;
  const std::shared_ptr<AsyncRequestState> result = state_map_iter->second;
  state_map_.erase(state_map_iter);
  return result;
}

std::vector<std::shared_ptr<AsyncRequestState>> AsyncRequestsStorage::PopAll() {
  const std::unique_lock<std::mutex> lock(mutex_);

  std::vector<std::shared_ptr<AsyncRequestState>> result;
  for (auto& request_id_and_state : state_map_)
    result.push_back(std::move(request_id_and_state.second));
  state_map_.clear();
  return result;
}

}  // namespace google_smart_card
