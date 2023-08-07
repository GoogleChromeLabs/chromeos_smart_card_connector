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

#ifndef GOOGLE_SMART_CARD_INTEGRATION_TESTING_INTEGRATION_TEST_SERVICE_H_
#define GOOGLE_SMART_CARD_INTEGRATION_TESTING_INTEGRATION_TEST_SERVICE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common/cpp/src/google_smart_card_common/global_context.h"
#include "common/cpp/src/google_smart_card_common/messaging/typed_message_listener.h"
#include "common/cpp/src/google_smart_card_common/requesting/js_request_receiver.h"
#include "common/cpp/src/google_smart_card_common/value.h"
#include <google_smart_card_integration_testing/integration_test_helper.h>

namespace google_smart_card {

// Singleton service class for handling JavaScript-and-C++ integration test
// scenarios.
class IntegrationTestService final : public RequestHandler {
 public:
  IntegrationTestService(const IntegrationTestService&) = delete;
  IntegrationTestService& operator=(const IntegrationTestService&) = delete;

  static IntegrationTestService* GetInstance();
  // Registers the given helper in the singleton instance, allowing JavaScript
  // test code to make calls to it once the singleton is activated.
  static IntegrationTestHelper* RegisterHelper(
      std::unique_ptr<IntegrationTestHelper> helper);

  // Starts listening for incoming requests and translating them into commands
  // for test helpers.
  void Activate(GlobalContext* global_context,
                TypedMessageRouter* typed_message_router);
  // Stops listening for incoming requests and clears internal state.
  //
  // All previously setup helpers must be torn down before calling this method.
  void Deactivate();

  // RequestHandler:
  void HandleRequest(Value payload,
                     RequestReceiver::ResultCallback result_callback) override;

 private:
  IntegrationTestService();
  ~IntegrationTestService() override;

  IntegrationTestHelper* FindHelperByName(const std::string& name);
  void SetUpHelper(const std::string& helper_name,
                   Value data_for_helper,
                   RequestReceiver::ResultCallback result_callback);
  void TearDownAllHelpers(RequestReceiver::ResultCallback result_callback);
  void SendMessageToHelper(const std::string& helper_name,
                           Value message_for_helper,
                           RequestReceiver::ResultCallback result_callback);

  GlobalContext* global_context_ = nullptr;
  TypedMessageRouter* typed_message_router_ = nullptr;
  std::shared_ptr<JsRequestReceiver> js_request_receiver_;
  std::vector<std::unique_ptr<IntegrationTestHelper>> helpers_;
  std::set<IntegrationTestHelper*> set_up_helpers_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_INTEGRATION_TESTING_INTEGRATION_TEST_SERVICE_H_
