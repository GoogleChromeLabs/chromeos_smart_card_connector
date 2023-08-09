// Copyright 2023 Google Inc. All Rights Reserved.
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

#include "common/integration_testing/src/google_smart_card_integration_testing/integration_test_helper.h"

#include <functional>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/requesting/request_receiver.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/value.h"

namespace google_smart_card {

IntegrationTestHelper::~IntegrationTestHelper() = default;

void IntegrationTestHelper::SetUp(
    GlobalContext* /*global_context*/,
    TypedMessageRouter* /*typed_message_router*/,
    Value /*data*/,
    RequestReceiver::ResultCallback result_callback) {
  result_callback(GenericRequestResult::CreateSuccessful(Value()));
}

void IntegrationTestHelper::TearDown(
    std::function<void()> completion_callback) {
  completion_callback();
}

}  // namespace google_smart_card
