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
  TearDownAllHelpers();
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
    SetUpHelper(helper_name, std::move(data_for_helper));
    result_callback(GenericRequestResult::CreateSuccessful(Value()));
    return;
  }
  if (request.function_name == "TearDownAll") {
    ExtractRemoteCallArgumentsOrDie(std::move(request.function_name),
                                    std::move(request.arguments));
    TearDownAllHelpers();
    result_callback(GenericRequestResult::CreateSuccessful(Value()));
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

void IntegrationTestService::SetUpHelper(const std::string& helper_name,
                                         Value data_for_helper) {
  IntegrationTestHelper* helper = FindHelperByName(helper_name);
  if (!helper)
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown helper " << helper_name;
  GOOGLE_SMART_CARD_CHECK(!set_up_helpers_.count(helper));
  set_up_helpers_.insert(helper);
  helper->SetUp(global_context_, typed_message_router_,
                std::move(data_for_helper));
}

void IntegrationTestService::TearDownAllHelpers() {
  for (auto* helper : set_up_helpers_)
    helper->TearDown();
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
