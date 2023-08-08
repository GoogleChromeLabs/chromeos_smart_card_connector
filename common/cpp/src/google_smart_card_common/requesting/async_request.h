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

// This file contains definitions of types related to keeping the asynchronous
// requests state.

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_ASYNC_REQUEST_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_ASYNC_REQUEST_H_

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "common/cpp/src/google_smart_card_common/logging/logging.h"
#include "common/cpp/src/google_smart_card_common/requesting/request_result.h"
#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

//
// Consumer-provided callback that will be called once the asynchronous request
// finishes (either successfully or not).
//

template <typename PayloadType>
using AsyncRequestCallback =
    std::function<void(RequestResult<PayloadType> request_result)>;

using GenericAsyncRequestCallback = AsyncRequestCallback<Value>;

// This class contains the internal state of an asynchronous request.
//
// Usually exists as a ref-counted object hidden from the consumer under the
// AsyncRequest class instance.
template <typename PayloadType>
class AsyncRequestState final {
 public:
  explicit AsyncRequestState(AsyncRequestCallback<PayloadType> callback)
      : callback_(callback) {
    GOOGLE_SMART_CARD_CHECK(callback_);
  }
  AsyncRequestState(const AsyncRequestState&) = delete;
  AsyncRequestState& operator=(const AsyncRequestState&) = delete;
  ~AsyncRequestState() = default;

  // Sets the result of the request, unless it was already set before.
  //
  // If the result was successfully set, then the callback passed to the class
  // constructor is executed.
  bool SetResult(RequestResult<PayloadType> request_result) {
    if (!is_callback_call_started_.test_and_set()) {
      callback_(std::move(request_result));
      return true;
    }
    return false;
  }

  bool SetCanceledResult() {
    return SetResult(RequestResult<PayloadType>::CreateCanceled());
  }

 private:
  const AsyncRequestCallback<PayloadType> callback_;
  std::atomic_flag is_callback_call_started_ = ATOMIC_FLAG_INIT;
};

using GenericAsyncRequestState = AsyncRequestState<Value>;

// This class contains the interface of an asynchronous request that is exposed
// to consumers.
//
// Note that this class has no methods for obtaining the request result: the
// results are delivered through the AsyncRequestCallback callback supplied when
// the request was sent.
template <typename PayloadType>
class AsyncRequest final {
 public:
  AsyncRequest() = default;
  AsyncRequest(const AsyncRequest&) = default;
  ~AsyncRequest() = default;

  explicit AsyncRequest(std::shared_ptr<AsyncRequestState<PayloadType>> state)
      : state_(state) {
    GOOGLE_SMART_CARD_CHECK(state_);
  }

  // Thread-safe assignment operator.
  AsyncRequest& operator=(const AsyncRequest& other) {
    std::atomic_store(&state_, other.state_);
    return *this;
  }

  // Cancels the request in a thread-safe manner.
  //
  // Returns whether the cancellation was successful. The cancellation fails if
  // the request has already finished with some result (including, but not
  // limiting to, another cancellation).
  bool Cancel() {
    const std::shared_ptr<AsyncRequestState<PayloadType>> state =
        std::atomic_load(&state_);
    GOOGLE_SMART_CARD_CHECK(state);
    return state->SetCanceledResult();
  }

 private:
  std::shared_ptr<AsyncRequestState<PayloadType>> state_;
};

using GenericAsyncRequest = AsyncRequest<Value>;

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_ASYNC_REQUEST_H_
