// Copyright 2022 Google Inc. All Rights Reserved.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <winscard.h>

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

#include "common/cpp/src/public/testing_global_context.h"
#include "smart_card_connector_app/src/testing_smart_card_simulation.h"

#ifdef __native_client__
#include <google_smart_card_common/nacl_io_utils.h>
#endif  // __native_client__

namespace google_smart_card {

class SmartCardConnectorApplicationTest : public ::testing::Test {
 protected:
  SmartCardConnectorApplicationTest() {
#ifdef __native_client__
    // Make resource files accessible.
    MountNaclIoFolders();
#endif  // __native_client__
    SetUpUsbSimulation();
    StartApplication();
  }

  ~SmartCardConnectorApplicationTest() { application_->ShutDownAndWait(); }

 private:
  void SetUpUsbSimulation() {
    global_context_.RegisterRequestHandler(
        TestingSmartCardSimulation::kRequesterName,
        std::bind(&TestingSmartCardSimulation::OnRequestToJs,
                  &smart_card_simulation_, std::placeholders::_1,
                  std::placeholders::_2));
  }

  void StartApplication() {
    // Set up the expectation on the first C++-to-JS message.
    auto pcsc_lite_ready_message_waiter = global_context_.CreateMessageWaiter(
        /*awaited_message_type=*/"pcsc_lite_ready");
    // Set up the expectation for the application to run the provided callback.
    ::testing::MockFunction<void()> background_initialization_callback;
    EXPECT_CALL(background_initialization_callback, Call());
    // Create the application, which spawns the background initialization
    // thread.
    application_ = MakeUnique<Application>(
        &global_context_, &typed_message_router_,
        background_initialization_callback.AsStdFunction());
    // Wait until the daemon's background thread completes the initialization
    // and notifies the JS side.
    pcsc_lite_ready_message_waiter->Wait();
    EXPECT_TRUE(pcsc_lite_ready_message_waiter->value().StrictlyEquals(
        Value(Value::Type::kDictionary)));
  }

  TypedMessageRouter typed_message_router_;
  TestingSmartCardSimulation smart_card_simulation_{&typed_message_router_};
  TestingGlobalContext global_context_{&typed_message_router_};
  std::unique_ptr<Application> application_;
};

TEST_F(SmartCardConnectorApplicationTest, SmokeTest) {}

// Test a PC/SC-Lite context can be established and freed, via direct C function
// calls `SCardEstablishContext()` and `SCardReleaseContext()`.
// This is an extended version of the "smoke test" as it verifies the daemon
// successfully started and replies to calls sent over (fake) sockets.
TEST_F(SmartCardConnectorApplicationTest, InternalApiContextEstablishing) {
  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SCardEstablishContext(SCARD_SCOPE_SYSTEM, /*pvReserved1=*/nullptr,
                                  /*pvReserved2=*/nullptr, &scard_context),
            SCARD_S_SUCCESS);
  EXPECT_NE(scard_context, 0);

  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);
}

}  // namespace google_smart_card
