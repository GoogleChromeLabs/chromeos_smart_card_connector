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

#include "common/cpp/src/google_smart_card_common/requesting/async_requests_storage.h"

#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "common/cpp/src/google_smart_card_common/requesting/async_request.h"
#include "common/cpp/src/google_smart_card_common/requesting/request_id.h"
#include "common/cpp/src/google_smart_card_common/requesting/request_result.h"

namespace google_smart_card {

namespace {

class TestAsyncRequestCallback {
 public:
  void operator()(GenericRequestResult /*request_result*/) {}
};

}  // namespace

TEST(RequestingAsyncRequestsStorageTest, Basic) {
  GenericAsyncRequestsStorage storage;

  // Initially the storage contains no requests
  EXPECT_FALSE(storage.Pop(static_cast<RequestId>(0)));
  EXPECT_TRUE(storage.PopAll().empty());

  // Two new request states are added, and they receive different identifiers
  std::shared_ptr<GenericAsyncRequestState> request_1_state(
      new GenericAsyncRequestState(TestAsyncRequestCallback()));
  const RequestId request_1_id = storage.Push(request_1_state);
  std::shared_ptr<GenericAsyncRequestState> request_2_state(
      new GenericAsyncRequestState(TestAsyncRequestCallback()));
  const RequestId request_2_id = storage.Push(request_2_state);
  EXPECT_NE(request_1_id, request_2_id);

  // Extraction of the two requests leaves the storage in an empty state again
  EXPECT_EQ(request_1_state, storage.Pop(request_1_id));
  EXPECT_EQ(request_2_state, storage.Pop(request_2_id));
  EXPECT_FALSE(storage.Pop(request_1_id));
  EXPECT_FALSE(storage.Pop(request_2_id));
  EXPECT_TRUE(storage.PopAll().empty());

  // Two new added requests receive identifiers distinct from the previous
  // requests' identifiers
  std::shared_ptr<GenericAsyncRequestState> request_3_state(
      new GenericAsyncRequestState(TestAsyncRequestCallback()));
  const RequestId request_3_id = storage.Push(request_3_state);
  EXPECT_NE(request_1_id, request_3_id);
  EXPECT_NE(request_2_id, request_3_id);
  std::shared_ptr<GenericAsyncRequestState> request_4_state(
      new GenericAsyncRequestState(TestAsyncRequestCallback()));
  const RequestId request_4_id = storage.Push(request_4_state);
  EXPECT_NE(request_1_id, request_4_id);
  EXPECT_NE(request_2_id, request_4_id);
  EXPECT_NE(request_3_id, request_4_id);

  // The two currently added requests can be extracted from the storage, but
  // they are returned not in any specific order
  const std::vector<std::shared_ptr<GenericAsyncRequestState>> requests =
      storage.PopAll();
  ASSERT_EQ(static_cast<size_t>(2), requests.size());
  EXPECT_TRUE(requests[0] == request_3_state &&
                  requests[1] == request_4_state ||
              requests[0] == request_4_state && requests[1] == request_3_state);
  EXPECT_TRUE(storage.PopAll().empty());
  EXPECT_FALSE(storage.Pop(request_3_id));
  EXPECT_FALSE(storage.Pop(request_4_id));
}

TEST(RequestingAsyncRequestsStorageTest, MultiThreading) {
  const int kThreadCount = 10;
  const int kIterationCount = 10 * 1000;

  GenericAsyncRequestsStorage storage;

  std::vector<std::thread> threads;
  for (int thread_index = 0; thread_index < kThreadCount; ++thread_index) {
    threads.emplace_back([&storage, thread_index] {
      std::vector<RequestId> request_ids;
      for (int iteration = 0; iteration < kIterationCount; ++iteration) {
        request_ids.push_back(
            storage.Push(std::make_shared<GenericAsyncRequestState>(
                TestAsyncRequestCallback())));
      }
      if (thread_index % 2 == 0) {
        for (auto request_id : request_ids)
          storage.Pop(request_id);
      } else {
        storage.PopAll();
      }
    });
  }

  for (int thread_index = 0; thread_index < kThreadCount; ++thread_index)
    threads[thread_index].join();

  EXPECT_TRUE(storage.PopAll().empty());
}

}  // namespace google_smart_card
