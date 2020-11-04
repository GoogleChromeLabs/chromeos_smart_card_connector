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

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/requesting/js_request_receiver.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>
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
    pp::Instance* pp_instance,
    pp::Core* pp_core,
    TypedMessageRouter* typed_message_router) {
  GOOGLE_SMART_CARD_CHECK(pp_instance);
  GOOGLE_SMART_CARD_CHECK(pp_core);
  GOOGLE_SMART_CARD_CHECK(typed_message_router);
  GOOGLE_SMART_CARD_CHECK(!pp_instance_);
  GOOGLE_SMART_CARD_CHECK(!pp_core_);
  GOOGLE_SMART_CARD_CHECK(!typed_message_router_);
  GOOGLE_SMART_CARD_CHECK(!js_request_receiver_);
  pp_instance_ = pp_instance;
  pp_core_ = pp_core;
  typed_message_router_ = typed_message_router;
  js_request_receiver_ = std::make_shared<JsRequestReceiver>(
      kIntegrationTestServiceRequesterName,
      /*request_handler=*/this, typed_message_router_,
      MakeUnique<JsRequestReceiver::PpDelegateImpl>(pp_instance_));
}

void IntegrationTestService::Deactivate() {
  GOOGLE_SMART_CARD_CHECK(js_request_receiver_);
  TearDownAllHelpers();
  js_request_receiver_.reset();
  typed_message_router_ = nullptr;
  pp_core_ = nullptr;
  pp_instance_ = nullptr;
}

void IntegrationTestService::HandleRequest(
    Value payload,
    RequestReceiver::ResultCallback result_callback) {
  RemoteCallRequestPayload request =
      ConvertFromValueOrDie<RemoteCallRequestPayload>(std::move(payload));
  // TODO(#185): Pass `Value`s, instead of converting into `pp::VarArray`.
  pp::VarArray arguments_var(
      ConvertValueToPpVar(ConvertToValueOrDie(std::move(request.arguments))));
  if (request.function_name == "SetUp") {
    std::string helper_name;
    pp::Var data_for_helper;
    GetVarArrayItems(arguments_var, &helper_name, &data_for_helper);
    SetUpHelper(helper_name, data_for_helper);
    result_callback(GenericRequestResult::CreateSuccessful(Value()));
    return;
  }
  if (request.function_name == "TearDownAll") {
    GOOGLE_SMART_CARD_CHECK(arguments_var.GetLength() == 0);
    TearDownAllHelpers();
    result_callback(GenericRequestResult::CreateSuccessful(Value()));
    return;
  }
  if (request.function_name == "HandleMessage") {
    std::string helper_name;
    pp::Var message_for_helper;
    GetVarArrayItems(arguments_var, &helper_name, &message_for_helper);
    SendMessageToHelper(helper_name, message_for_helper, result_callback);
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
                                         const pp::Var& data_for_helper) {
  IntegrationTestHelper* helper = FindHelperByName(helper_name);
  if (!helper)
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown helper " << helper_name;
  GOOGLE_SMART_CARD_CHECK(!set_up_helpers_.count(helper));
  set_up_helpers_.insert(helper);
  helper->SetUp(pp_instance_, pp_core_, typed_message_router_, data_for_helper);
}

void IntegrationTestService::TearDownAllHelpers() {
  for (auto* helper : set_up_helpers_)
    helper->TearDown();
  set_up_helpers_.clear();
}

void IntegrationTestService::SendMessageToHelper(
    const std::string& helper_name,
    const pp::Var& message_for_helper,
    RequestReceiver::ResultCallback result_callback) {
  IntegrationTestHelper* helper = FindHelperByName(helper_name);
  if (!helper)
    GOOGLE_SMART_CARD_LOG_FATAL << "Unknown helper " << helper_name;
  GOOGLE_SMART_CARD_CHECK(set_up_helpers_.count(helper));
  helper->OnMessageFromJs(message_for_helper, result_callback);
}

}  // namespace google_smart_card
