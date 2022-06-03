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
#include <mutex>
#include <queue>
#include <string>
#include <utility>
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

using testing::ElementsAre;
using testing::IsEmpty;

namespace google_smart_card {

namespace {

// Records reader_* messages sent to JS and allows to inspect them in tests.
class ReaderNotificationObserver final {
 public:
  void Init(TestingGlobalContext& global_context) {
    for (const auto* event_name :
         {"reader_init_add", "reader_finish_add", "reader_remove"}) {
      global_context.RegisterMessageHandler(
          event_name,
          std::bind(&ReaderNotificationObserver::OnMessageToJs, this,
                    event_name, /*request_payload=*/std::placeholders::_1,
                    /*request_id=*/std::placeholders::_2));
    }
  }

  // Extracts the next recorded notification, in the format "<event>:<reader>"
  // (for simplifying test assertions).
  std::string Pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    GOOGLE_SMART_CARD_CHECK(!recorded_notifications_.empty());
    std::string next = std::move(recorded_notifications_.front());
    recorded_notifications_.pop();
    return next;
  }

  // Same as `Pop()`, but waits if there's no notification to return.
  std::string WaitAndPop() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [&] {
      return !recorded_notifications_.empty();
    });
    std::string next = std::move(recorded_notifications_.front());
    recorded_notifications_.pop();
    return next;
  }

  // Returns whether there's a notification to return.
  bool Empty() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return recorded_notifications_.empty();
  }

 private:
  void OnMessageToJs(const std::string& event_name,
                     Value message_data,
                     optional<RequestId> /*request_id*/) {
    std::string notification =
        event_name + ":" +
        message_data.GetDictionaryItem("readerName")->GetString();

    std::unique_lock<std::mutex> lock(mutex_);
    recorded_notifications_.push(notification);
    condition_.notify_one();
  }

  // Mutable to allow locking in const methods.
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<std::string> recorded_notifications_;
};

std::vector<std::string> DirectCallSCardListReaders(
    SCARDCONTEXT scard_context) {
  DWORD readers_size = 0;
  LONG return_code = SCardListReaders(scard_context, /*mszGroups=*/nullptr,
                                      /*mszReaders=*/nullptr, &readers_size);
  if (return_code == SCARD_E_NO_READERS_AVAILABLE)
    return {};
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_GT(readers_size, 0U);

  std::string readers_multistring;
  readers_multistring.resize(readers_size);
  EXPECT_EQ(SCardListReaders(scard_context, /*mszGroups=*/nullptr,
                             &readers_multistring[0], &readers_size),
            SCARD_S_SUCCESS);
  EXPECT_EQ(readers_size, readers_multistring.size());

  return ExtractMultiStringElements(readers_multistring);
}

}  // namespace

class SmartCardConnectorApplicationTest : public ::testing::Test {
 protected:
  SmartCardConnectorApplicationTest() {
#ifdef __native_client__
    // Make resource files accessible.
    MountNaclIoFolders();
#endif  // __native_client__
    SetUpUsbSimulation();
    reader_notification_observer_.Init(global_context_);
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

  ReaderNotificationObserver& reader_notification_observer() {
    return reader_notification_observer_;
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

  TypedMessageRouter typed_message_router_;
  TestingSmartCardSimulation smart_card_simulation_{&typed_message_router_};
  ReaderNotificationObserver reader_notification_observer_;
  TestingGlobalContext global_context_{&typed_message_router_};
  std::unique_ptr<Application> application_;
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
  // No need to wait here, since the notifications for the initially present
  // devices are sent during the startup.
  EXPECT_EQ(reader_notification_observer().Pop(),
            "reader_init_add:Gemalto PC Twin Reader");
  EXPECT_EQ(reader_notification_observer().Pop(),
            "reader_finish_add:Gemalto PC Twin Reader");
  EXPECT_TRUE(reader_notification_observer().Empty());

  // Act:

  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SCardEstablishContext(SCARD_SCOPE_SYSTEM, /*pvReserved1=*/nullptr,
                                  /*pvReserved2=*/nullptr, &scard_context),
            SCARD_S_SUCCESS);
  std::vector<std::string> readers = DirectCallSCardListReaders(scard_context);
  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);

  // Assert:

  EXPECT_THAT(readers, ElementsAre("Gemalto PC Twin Reader 00 00"));
}

TEST_F(SmartCardConnectorApplicationTest,
       InternalApiGetStatusChangeDeviceAppearing) {
  // Arrange:

  // Start with an empty list of readers.
  StartApplication();
  EXPECT_TRUE(reader_notification_observer().Empty());

  // Act:

  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SCardEstablishContext(SCARD_SCOPE_SYSTEM, /*pvReserved1=*/nullptr,
                                  /*pvReserved2=*/nullptr, &scard_context),
            SCARD_S_SUCCESS);
  EXPECT_TRUE(DirectCallSCardListReaders(scard_context).empty());

  // Simulate connecting a reader.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});

  // Wait until PC/SC reports a change in the list of readers.
  std::vector<SCARD_READERSTATE> reader_states(1);
  reader_states[0].szReader = R"(\\?PnP?\Notification)";
  reader_states[0].dwCurrentState = SCARD_STATE_UNAWARE;
  EXPECT_EQ(SCardGetStatusChange(scard_context, /*dwTimeout=*/INFINITE,
                                 reader_states.data(), reader_states.size()),
            SCARD_S_SUCCESS);

  std::vector<std::string> readers = DirectCallSCardListReaders(scard_context);

  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);

  // Assert:

  EXPECT_THAT(readers, ElementsAre("Gemalto PC Twin Reader 00 00"));
  EXPECT_EQ(reader_notification_observer().WaitAndPop(),
            "reader_init_add:Gemalto PC Twin Reader");
  EXPECT_EQ(reader_notification_observer().WaitAndPop(),
            "reader_finish_add:Gemalto PC Twin Reader");
  EXPECT_TRUE(reader_notification_observer().Empty());
}

TEST_F(SmartCardConnectorApplicationTest,
       InternalApiGetStatusChangeDeviceRemoving) {
  // Arrange:

  // Start with a single reader.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});

  StartApplication();
  // No need to wait here, since the notifications for the initially present
  // devices are sent during the startup.
  EXPECT_EQ(reader_notification_observer().Pop(),
            "reader_init_add:Gemalto PC Twin Reader");
  EXPECT_EQ(reader_notification_observer().Pop(),
            "reader_finish_add:Gemalto PC Twin Reader");
  EXPECT_TRUE(reader_notification_observer().Empty());

  // Act:

  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SCardEstablishContext(SCARD_SCOPE_SYSTEM, /*pvReserved1=*/nullptr,
                                  /*pvReserved2=*/nullptr, &scard_context),
            SCARD_S_SUCCESS);

  EXPECT_THAT(DirectCallSCardListReaders(scard_context),
              ElementsAre("Gemalto PC Twin Reader 00 00"));

  // Simulate disconnecting the reader.
  SetUsbDevices({});

  // Wait until PC/SC reports a change in the list of readers.
  std::vector<SCARD_READERSTATE> reader_states(1);
  reader_states[0].szReader = R"(\\?PnP?\Notification)";
  reader_states[0].dwCurrentState = SCARD_STATE_UNAWARE;
  EXPECT_EQ(SCardGetStatusChange(scard_context, /*dwTimeout=*/INFINITE,
                                 reader_states.data(), reader_states.size()),
            SCARD_S_SUCCESS);

  std::vector<std::string> readers = DirectCallSCardListReaders(scard_context);

  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);

  // Assert:

  EXPECT_THAT(readers, IsEmpty());
  EXPECT_EQ(reader_notification_observer().WaitAndPop(),
            "reader_remove:Gemalto PC Twin Reader");
  EXPECT_TRUE(reader_notification_observer().Empty());
}

}  // namespace google_smart_card
