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
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <winscard.h>

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/multi_string.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

#include "common/cpp/src/public/testing_global_context.h"
#include "smart_card_connector_app/src/testing_smart_card_simulation.h"

#ifdef __native_client__
#include <google_smart_card_common/nacl_io_utils.h>
#endif  // __native_client__

#ifdef __native_client__
#include <google_smart_card_common/nacl_io_utils.h>
#endif  // __native_client__

using testing::ElementsAre;

namespace google_smart_card {

class SmartCardConnectorApplicationTest : public ::testing::Test {
 protected:
  SmartCardConnectorApplicationTest() {
#ifdef __native_client__
    // Make resource files accessible.
    MountNaclIoFolders();
#endif  // __native_client__
    SetUpUsbSimulation();
    SetUpReaderMessageObserving();
  }

  ~SmartCardConnectorApplicationTest() { application_->ShutDownAndWait(); }

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

  // Enables the specified fake USB devices.
  void SetUsbDevices(
      const std::vector<TestingSmartCardSimulation::Device>& devices) {
    smart_card_simulation_.SetDevices(devices);
  }

  // Returns all "reader_init_add" messages that were sent to JS (they are sent
  // when a new reader starts to be initialized).
  const std::vector<std::string>& reader_init_add_messages() const {
    return reader_init_add_messages_;
  }
  // Returns all "reader_finish_add" messages that were sent to JS (they are
  // sent when a reader initialization finishes).
  const std::vector<std::string>& reader_finish_add_messages() const {
    return reader_finish_add_messages_;
  }

 private:
  void SetUpUsbSimulation() {
    global_context_.RegisterRequestHandler(
        TestingSmartCardSimulation::kRequesterName,
        std::bind(&TestingSmartCardSimulation::OnRequestToJs,
                  &smart_card_simulation_,
                  /*request_payload=*/std::placeholders::_1,
                  /*request_id=*/std::placeholders::_2));
  }

  void SetUpReaderMessageObserving() {
    global_context_.RegisterMessageHandler(
        "reader_init_add",
        std::bind(&SmartCardConnectorApplicationTest::OnReaderInitAddPosted,
                  this, /*request_payload=*/std::placeholders::_1,
                  /*request_id=*/std::placeholders::_2));
    global_context_.RegisterMessageHandler(
        "reader_finish_add",
        std::bind(&SmartCardConnectorApplicationTest::OnReaderFinishAddPosted,
                  this, /*request_payload=*/std::placeholders::_1,
                  /*request_id=*/std::placeholders::_2));
  }

  // Called when the code-under-test sends a "reader_init_add" message to JS.
  void OnReaderInitAddPosted(Value request_payload,
                             optional<RequestId> /*request_id*/) {
    reader_init_add_messages_.push_back(
        request_payload.GetDictionaryItem("readerName")->GetString());
  }

  // Called when the code-under-test sends a "reader_finish_add" message to JS.
  void OnReaderFinishAddPosted(Value request_payload,
                               optional<RequestId> /*request_id*/) {
    reader_finish_add_messages_.push_back(
        request_payload.GetDictionaryItem("readerName")->GetString());
  }

  TypedMessageRouter typed_message_router_;
  TestingSmartCardSimulation smart_card_simulation_{&typed_message_router_};
  TestingGlobalContext global_context_{&typed_message_router_};
  std::unique_ptr<Application> application_;
  std::vector<std::string> reader_init_add_messages_;
  std::vector<std::string> reader_finish_add_messages_;
};

TEST_F(SmartCardConnectorApplicationTest, SmokeTest) {
  StartApplication();
}

// Test a PC/SC-Lite context can be established and freed, via direct C function
// calls `SCardEstablishContext()` and `SCardReleaseContext()`.
// This is an extended version of the "smoke test" as it verifies the daemon
// successfully started and replies to calls sent over (fake) sockets.
TEST_F(SmartCardConnectorApplicationTest, InternalApiContextEstablishing) {
  // Arrange:
  StartApplication();

  // Act:

  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SCardEstablishContext(SCARD_SCOPE_SYSTEM, /*pvReserved1=*/nullptr,
                                  /*pvReserved2=*/nullptr, &scard_context),
            SCARD_S_SUCCESS);
  EXPECT_NE(scard_context, 0);

  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);
}

// Test a single reader is successfully initialized by PC/SC-Lite and is
// returned via the direct C function call `SCardListReaders()`.
TEST_F(SmartCardConnectorApplicationTest, InternalApiSingleDeviceListing) {
  // Arrange:

  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});

  StartApplication();
  EXPECT_THAT(reader_init_add_messages(),
              ElementsAre("Gemalto PC Twin Reader"));
  EXPECT_THAT(reader_finish_add_messages(),
              ElementsAre("Gemalto PC Twin Reader"));

  // Act:

  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SCardEstablishContext(SCARD_SCOPE_SYSTEM, /*pvReserved1=*/nullptr,
                                  /*pvReserved2=*/nullptr, &scard_context),
            SCARD_S_SUCCESS);

  DWORD readers_size = 0;
  EXPECT_EQ(SCardListReaders(scard_context, /*mszGroups=*/nullptr,
                             /*mszReaders=*/nullptr, &readers_size),
            SCARD_S_SUCCESS);

  std::string readers_multistring;
  readers_multistring.resize(readers_size);
  EXPECT_EQ(SCardListReaders(scard_context, /*mszGroups=*/nullptr,
                             &readers_multistring[0], &readers_size),
            SCARD_S_SUCCESS);
  EXPECT_THAT(ExtractMultiStringElements(readers_multistring),
              ElementsAre("Gemalto PC Twin Reader 00 00"));

  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);
}

}  // namespace google_smart_card
