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

#include <chrono>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

namespace {

class TestAsyncRequestCallback {
 public:
  void operator()(GenericRequestResult request_result) {
    request_result_ = std::move(request_result);
    ++call_count_;
  }

  int call_count() const { return call_count_; }

  const GenericRequestResult& request_result() const { return request_result_; }

 private:
  int call_count_ = 0;
  GenericRequestResult request_result_;
};

}  // namespace

TEST(RequestingAsyncRequestTest, AsyncRequestStateBasic) {
  TestAsyncRequestCallback callback;

  // Initially the request state is constructed with no request result, and the
  // callback is not executed
  GenericAsyncRequestState request_state(std::ref(callback));
  ASSERT_EQ(0, callback.call_count());

  // The first set of the request result is successful and triggers the callback
  const int kValue = 123;
  ASSERT_TRUE(request_state.SetResult(
      GenericRequestResult::CreateSuccessful(Value(kValue))));
  ASSERT_EQ(1, callback.call_count());
  ASSERT_TRUE(callback.request_result().payload().is_integer());
  EXPECT_EQ(callback.request_result().payload().GetInteger(), kValue);

  // The subsequent set of the request result does not change the stored value
  // and does not trigger the callback
  ASSERT_FALSE(request_state.SetResult(GenericRequestResult::CreateFailed("")));
  ASSERT_EQ(1, callback.call_count());
  ASSERT_TRUE(callback.request_result().payload().is_integer());
  EXPECT_EQ(callback.request_result().payload().GetInteger(), kValue);
}

#ifdef __EMSCRIPTEN__
// TODO(#185): Crashes in Emscripten due to out-of-memory.
#define MAYBE_AsyncRequestStateMultiThreading \
  DISABLED_AsyncRequestStateMultiThreading
#else
#define MAYBE_AsyncRequestStateMultiThreading AsyncRequestStateMultiThreading
#endif
TEST(RequestingAsyncRequestTest, MAYBE_AsyncRequestStateMultiThreading) {
  const int kIterationCount = 300;
  const int kStateCount = 100;
  const int kThreadCount = 10;
  const auto kThreadsStartTimeout = std::chrono::milliseconds(10);

  for (int iteration = 0; iteration < kIterationCount; ++iteration) {
    std::vector<TestAsyncRequestCallback> callbacks(kStateCount);
    std::vector<std::unique_ptr<GenericAsyncRequestState>> states;
    for (int index = 0; index < kStateCount; ++index) {
      states.emplace_back(
          new GenericAsyncRequestState(std::ref(callbacks[index])));
    }

    std::vector<std::thread> threads;
    const auto threads_start_time =
        std::chrono::high_resolution_clock::now() + kThreadsStartTimeout;
    for (int thread_index = 0; thread_index < kThreadCount; ++thread_index) {
      threads.emplace_back([&states, threads_start_time] {
        std::this_thread::sleep_until(threads_start_time);
        for (int index = 0; index < kStateCount; ++index)
          states[index]->SetResult(GenericRequestResult::CreateFailed(""));
      });
    }
    for (int thread_index = 0; thread_index < kThreadCount; ++thread_index)
      threads[thread_index].join();

    for (int index = 0; index < kStateCount; ++index)
      EXPECT_EQ(1, callbacks[index].call_count());
  }
}

TEST(RequestingAsyncRequestTest, AsyncRequestBasic) {
  TestAsyncRequestCallback callback;

  // Initially the request is constructed with an empty request state
  const auto request_state =
      std::make_shared<GenericAsyncRequestState>(std::ref(callback));
  GenericAsyncRequest request(request_state);
  ASSERT_EQ(0, callback.call_count());

  // The request state receives the result, which triggers the callback
  ASSERT_TRUE(request_state->SetResult(GenericRequestResult::CreateFailed("")));
  ASSERT_EQ(1, callback.call_count());

  // After the result is already set, request cancellation has no effect
  request.Cancel();
  ASSERT_EQ(1, callback.call_count());
}

TEST(RequestingAsyncRequestTest, AsyncRequestCancellation) {
  TestAsyncRequestCallback callback;

  // Initially the request is constructed with an empty request state
  const auto request_state =
      std::make_shared<GenericAsyncRequestState>(std::ref(callback));
  GenericAsyncRequest request(request_state);
  ASSERT_EQ(0, callback.call_count());

  // The request is canceled, which sets the result to "canceled"
  request.Cancel();
  ASSERT_EQ(1, callback.call_count());
  EXPECT_TRUE(callback.request_result().status() ==
              RequestResultStatus::kCanceled);

  // After the request is canceled, request result assignment has no effect
  ASSERT_FALSE(
      request_state->SetResult(GenericRequestResult::CreateFailed("")));
  ASSERT_EQ(1, callback.call_count());
}

}  // namespace google_smart_card
