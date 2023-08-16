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

#include "smart_card_connector_app/src/application.h"

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/requesting/request_receiver.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/integration_testing/src/public/integration_test_helper.h"
#include "common/integration_testing/src/public/integration_test_service.h"
#include "smart_card_connector_app/src/testing_smart_card_simulation.h"

namespace google_smart_card {

// The helper that can be used in JS-to-C++ tests to run the core functionality
// of the Smart Card Connector application, i.e., the PC/SC server.
class SmartCardConnectorApplicationTestHelper final
    : public IntegrationTestHelper {
 public:
  // IntegrationTestHelper:
  std::string GetName() const override;
  void SetUp(GlobalContext* global_context,
             TypedMessageRouter* typed_message_router,
             Value data,
             RequestReceiver::ResultCallback result_callback) override;
  void TearDown(std::function<void()> completion_callback) override;
  void OnMessageFromJs(
      Value data,
      RequestReceiver::ResultCallback result_callback) override;

 private:
  void SetSimulatedUsbDevices(Value devices);

  std::unique_ptr<TestingSmartCardSimulation> smart_card_simulation_;
  std::unique_ptr<Application> application_;
};

// Register the class in the service, so that when the JS side requests this
// helper the service will route requests to it.
const auto g_smart_card_connector_application_test_helper =
    IntegrationTestService::RegisterHelper(
        MakeUnique<SmartCardConnectorApplicationTestHelper>());

std::string SmartCardConnectorApplicationTestHelper::GetName() const {
  return "SmartCardConnectorApplicationTestHelper";
}

void SmartCardConnectorApplicationTestHelper::SetUp(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router,
    Value data,
    RequestReceiver::ResultCallback result_callback) {
  smart_card_simulation_ = MakeUnique<TestingSmartCardSimulation>(
      global_context, typed_message_router);
  SetSimulatedUsbDevices(std::move(data));
  application_ = MakeUnique<Application>(global_context, typed_message_router,
                                         std::function<void()>());
  // Note: We don't wait until the application completes its initialization on
  // background threads, but the test can wait for it via ReadinessTracker.
  result_callback(GenericRequestResult::CreateSuccessful(Value()));
}

void SmartCardConnectorApplicationTestHelper::TearDown(
    std::function<void()> completion_callback) {
  // Perform the shutdown on a background thread, because it involves blocking
  // operations, but some environments (like Emscripten) forbid them on the main
  // thread.
  std::thread([this, completion_callback] {
    application_->ShutDownAndWait();
    application_.reset();
    smart_card_simulation_.reset();
    completion_callback();
  }).detach();
}

void SmartCardConnectorApplicationTestHelper::OnMessageFromJs(
    Value data,
    RequestReceiver::ResultCallback result_callback) {
  SetSimulatedUsbDevices(std::move(data));
  result_callback(GenericRequestResult::CreateSuccessful(Value()));
}

void SmartCardConnectorApplicationTestHelper::SetSimulatedUsbDevices(
    Value devices) {
  std::string error_message;
  std::vector<TestingSmartCardSimulation::Device> parsed_devices;
  if (!ConvertFromValue(std::move(devices), &parsed_devices, &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL
        << "Failed to parse simulation Device array from JS: " << error_message;
  }
  smart_card_simulation_->SetDevices(parsed_devices);
}

}  // namespace google_smart_card
