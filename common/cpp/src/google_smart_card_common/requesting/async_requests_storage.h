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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_ASYNC_REQUESTS_STORAGE_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_ASYNC_REQUESTS_STORAGE_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "common/cpp/src/google_smart_card_common/logging/logging.h"
#include "common/cpp/src/google_smart_card_common/requesting/async_request.h"
#include "common/cpp/src/google_smart_card_common/requesting/request_id.h"
#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

// Storage for asynchronous request states, that keeps them in a mapping based
// on a generated sequence of identifiers.
//
// This class is thread-safe.
template <typename PayloadType>
class AsyncRequestsStorage final {
 public:
  AsyncRequestsStorage() = default;

  AsyncRequestsStorage(const AsyncRequestsStorage&) = delete;
  AsyncRequestsStorage& operator=(const AsyncRequestsStorage&) = delete;

  ~AsyncRequestsStorage() = default;

  // Adds a new asynchronous request state under a unique identifier and returns
  // this identifier.
  RequestId Push(
      std::shared_ptr<AsyncRequestState<PayloadType>> async_request_state) {
    const std::unique_lock<std::mutex> lock(mutex_);

    const RequestId request_id = next_free_request_id_;
    ++next_free_request_id_;

    const bool is_new_state_added =
        state_map_.emplace(request_id, async_request_state).second;
    GOOGLE_SMART_CARD_CHECK(is_new_state_added);

    return request_id;
  }

  // Extracts the asynchronous request state that corresponds to the specified
  // identifier.
  //
  // Returns null if the specified request identifier is not present.
  std::shared_ptr<AsyncRequestState<PayloadType>> Pop(RequestId request_id) {
    const std::unique_lock<std::mutex> lock(mutex_);

    const auto state_map_iter = state_map_.find(request_id);
    if (state_map_iter == state_map_.end())
      return nullptr;
    const std::shared_ptr<AsyncRequestState<PayloadType>> result =
        state_map_iter->second;
    state_map_.erase(state_map_iter);
    return result;
  }

  // Extracts all stored asynchronous request states.
  //
  // The order of the returned request states is unspecified.
  std::vector<std::shared_ptr<AsyncRequestState<PayloadType>>> PopAll() {
    const std::unique_lock<std::mutex> lock(mutex_);

    std::vector<std::shared_ptr<AsyncRequestState<PayloadType>>> result;
    for (auto& request_id_and_state : state_map_)
      result.push_back(std::move(request_id_and_state.second));
    state_map_.clear();
    return result;
  }

 private:
  std::mutex mutex_;
  RequestId next_free_request_id_ = 0;
  std::unordered_map<RequestId,
                     const std::shared_ptr<AsyncRequestState<PayloadType>>>
      state_map_;
};

using GenericAsyncRequestsStorage = AsyncRequestsStorage<Value>;

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_ASYNC_REQUESTS_STORAGE_H_
