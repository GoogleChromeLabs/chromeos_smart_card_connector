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

#include "common/cpp/src/public/logging/logging.h"

#include <memory>
#include <string>

#include "common/cpp/src/public/requesting/request_receiver.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/value.h"
#include "common/integration_testing/src/public/integration_test_helper.h"
#include "common/integration_testing/src/public/integration_test_service.h"

namespace google_smart_card {

// The helper that can be used in JS-to-C++ tests to trigger the C++ logging
// functionality.
class LoggingTestHelper final : public IntegrationTestHelper {
 public:
  // IntegrationTestHelper:
  std::string GetName() const override;
  void OnMessageFromJs(
      Value data,
      RequestReceiver::ResultCallback result_callback) override;
};

// Register the class in the service, so that when the JS side requests this
// helper the service will route requests to it.
const auto g_smart_card_connector_application_test_helper =
    IntegrationTestService::RegisterHelper(
        std::make_unique<LoggingTestHelper>());

std::string LoggingTestHelper::GetName() const {
  return "LoggingTestHelper";
}

void LoggingTestHelper::OnMessageFromJs(
    Value data,
    RequestReceiver::ResultCallback result_callback) {
  if (data.GetString() == "crash-via-check") {
    GOOGLE_SMART_CARD_CHECK(0);
  } else if (data.GetString() == "crash-via-fatal-log") {
    GOOGLE_SMART_CARD_LOG_FATAL << "Intentional crash";
  } else {
    GOOGLE_SMART_CARD_LOG_FATAL << "";
  }
  result_callback(GenericRequestResult::CreateSuccessful(Value()));
}

}  // namespace google_smart_card
