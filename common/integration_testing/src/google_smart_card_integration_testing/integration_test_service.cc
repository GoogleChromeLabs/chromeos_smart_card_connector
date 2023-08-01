// Copyright 2020 Google Inc. All Rights Reserved.
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

#include <google_smart_card_integration_testing/integration_test_service.h>

#include <atomic>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/js_request_receiver.h>
#include <google_smart_card_common/requesting/remote_call_arguments_conversion.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_integration_testing/integration_test_helper.h>

namespace google_smart_card {

namespace {

constexpr char kIntegrationTestServiceRequesterName[] = "integration_test";

// Returns a callback that, after being called exactly `barrier_value` times,
// runs `original_callback`.
std::function<void()> MakeBarrierCallback(
    std::function<void()> original_callback,
    size_t barrier_value) {
  GOOGLE_SMART_CARD_CHECK(barrier_value > 0);
  auto counter = std::make_shared<std::atomic_int>(barrier_value);
  return [counter, original_callback]() {
    if (--*counter == 0)
      original_callback();
  };
}

}  // namespace

// static
IntegrationTestService* IntegrationTestService::GetInstance() {
  static IntegrationTestService instance;
  return &instance;
}

// static
IntegrationTestHelper* IntegrationTestService::RegisterHelper(
    std::unique_ptr<IntegrationTestHelper> helper) {
  IntegrationTestHelper* const helper_ptr = helper.get();
  GetInstance()->helpers_.push_back(std::move(helper));
  return helper_ptr;
}

void IntegrationTestService::Activate(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router) {
  GOOGLE_SMART_CARD_CHECK(global_context);
  GOOGLE_SMART_CARD_CHECK(typed_message_router);
  GOOGLE_SMART_CARD_CHECK(!global_context_);
  GOOGLE_SMART_CARD_CHECK(!typed_message_router_);
  GOOGLE_SMART_CARD_CHECK(!js_request_receiver_);
  global_context_ = global_context;
  typed_message_router_ = typed_message_router;
  js_request_receiver_ = std::make_shared<JsRequestReceiver>(
      kIntegrationTestServiceRequesterName,
      /*request_handler=*/this, global_context_, typed_message_router_);
}

void IntegrationTestService::Deactivate() {
  GOOGLE_SMART_CARD_CHECK(js_request_receiver_);
  // It's expected that all helpers have been torn down.
  GOOGLE_SMART_CARD_CHECK(set_up_helpers_.empty());
  js_request_receiver_.reset();
  typed_message_router_ = nullptr;
}

void IntegrationTestService::HandleRequest(
    Value payload,
    RequestReceiver::ResultCallback result_callback) {
  RemoteCallRequestPayload request =
      ConvertFromValueOrDie<RemoteCallRequestPayload>(std::move(payload));
  if (request.function_name == "SetUp") {
    std::string helper_name;
    Value data_for_helper;
    ExtractRemoteCallArgumentsOrDie(std::move(request.function_name),
                                    std::move(request.arguments), &helper_name,
                                    &data_for_helper);
    SetUpHelper(helper_name, std::move(data_for_helper), result_callback);
    return;
  }
  if (request.function_name == "TearDownAll") {
    ExtractRemoteCallArgumentsOrDie(std::move(request.function_name),
                                    std::move(request.arguments));
    TearDownAllHelpers(result_callback);
    return;
  }
  if (request.function_name == "HandleMessage") {
    std::string helper_name;
    Value message_for_helper;
    ExtractRemoteCallArgumentsOrDie(std::move(request.function_name),
                                    std::move(request.arguments), &helper_name,
                                    &message_for_helper);
    SendMessageToHelper(helper_name, std::move(message_for_helper),
                        result_callback);
    return;
  }
  GOOGLE_SMART_CARD_LOG_FATAL << "Unexpected method " << request.function_name;
}

IntegrationTestService::IntegrationTestService() = default;

IntegrationTestService::~IntegrationTestService() = default;

IntegrationTestHelper* IntegrationTestService::FindHelperByName(
    const std::string& name) {
  for (const auto& helper : helpers_) {
    if (helper->GetName() == name)
      return helper.get();
  }
  return nullptr;
}

void IntegrationTestService::SetUpHelper(
    const std::string& helper_name,
    Value data_for_helper,
    RequestReceiver::ResultCallback result_callback) {
  IntegrationTestHelper* helper = FindHelperByName(helper_name);
  if (!helper)
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown helper " << helper_name;
  GOOGLE_SMART_CARD_CHECK(!set_up_helpers_.count(helper));
  set_up_helpers_.insert(helper);
  helper->SetUp(global_context_, typed_message_router_,
                std::move(data_for_helper), result_callback);
}

void IntegrationTestService::TearDownAllHelpers(
    RequestReceiver::ResultCallback result_callback) {
  std::function<void()> canned_callback = [result_callback]() {
    result_callback(GenericRequestResult::CreateSuccessful(Value()));
  };
  if (set_up_helpers_.empty()) {
    canned_callback();
    return;
  }
  // If there are multiple active helpers, start tearing down each of them.
  // Report result via the callback once all helpers finish tearing down.
  auto barrier_callback =
      MakeBarrierCallback(canned_callback, set_up_helpers_.size());
  for (auto* helper : set_up_helpers_)
    helper->TearDown(barrier_callback);
  set_up_helpers_.clear();
}

void IntegrationTestService::SendMessageToHelper(
    const std::string& helper_name,
    Value message_for_helper,
    RequestReceiver::ResultCallback result_callback) {
  IntegrationTestHelper* helper = FindHelperByName(helper_name);
  if (!helper)
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown helper " << helper_name;
  GOOGLE_SMART_CARD_CHECK(set_up_helpers_.count(helper));
  helper->OnMessageFromJs(std::move(message_for_helper), result_callback);
}

}  // namespace google_smart_card
