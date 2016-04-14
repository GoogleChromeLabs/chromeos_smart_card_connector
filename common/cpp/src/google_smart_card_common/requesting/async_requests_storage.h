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

#include <google_smart_card_common/requesting/async_request.h>
#include <google_smart_card_common/requesting/request_id.h>

namespace google_smart_card {

// Storage for asynchronous request states, that keeps them in a mapping based
// on a generated sequence of identifiers.
//
// This class is thread-safe.
class AsyncRequestsStorage final {
 public:
  AsyncRequestsStorage();

  // Adds a new asynchronous request state under a unique identifier and returns
  // this identifier.
  RequestId Push(std::shared_ptr<AsyncRequestState> async_request_state);

  // Extracts the asynchronous request state that corresponds to the specified
  // identifier.
  //
  // Returns null if the specified request identifier is not present.
  std::shared_ptr<AsyncRequestState> Pop(RequestId request_id);

  // Extracts all stored asynchronous request states.
  //
  // The order of the returned request states is unspecified.
  std::vector<std::shared_ptr<AsyncRequestState>> PopAll();

 private:
  std::mutex mutex_;
  RequestId next_free_request_id_;
  std::unordered_map<RequestId, const std::shared_ptr<AsyncRequestState>>
  state_map_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_ASYNC_REQUESTS_STORAGE_H_
