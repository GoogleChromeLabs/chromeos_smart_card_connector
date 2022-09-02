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

#include <atomic>
#include <condition_variable>
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

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/multi_string.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/optional_test_utils.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/requesting/requester_message.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_test_utils.h>

#include "common/cpp/src/public/testing_global_context.h"
#include "common/cpp/src/public/value_builder.h"
#include "smart_card_connector_app/src/testing_smart_card_simulation.h"

#ifdef __native_client__
#include <google_smart_card_common/nacl_io_utils.h>
#endif  // __native_client__

using testing::AnyOf;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;

namespace google_smart_card {

namespace {

// The constant from the PC/SC-Lite API docs.
constexpr char kPnpNotification[] = R"(\\?PnP?\Notification)";
// Name of `TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader` as it
// appears in the PC/SC-Lite API. The "0" suffix corresponds to the "00 00" part
// that contains nonzeroes in case there are multiple devices with the same
// name.
constexpr char kGemaltoPcTwinReaderPcscName0[] = "Gemalto PC Twin Reader 00 00";

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
    condition_.wait(lock, [&] { return !recorded_notifications_.empty(); });
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
                     optional<Value> message_data,
                     optional<RequestId> /*request_id*/) {
    ASSERT_TRUE(message_data);
    std::string notification =
        event_name + ":" +
        message_data->GetDictionaryItem("readerName")->GetString();

    std::unique_lock<std::mutex> lock(mutex_);
    recorded_notifications_.push(notification);
    condition_.notify_one();
  }

  // Mutable to allow locking in const methods.
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<std::string> recorded_notifications_;
};

std::string GetJsClientRequesterName(int handler_id) {
  // The template should match the one in
  // third_party/pcsc-lite/naclport/server_clients_management/src/clients_manager.cc.
  // We hardcode it here too, so that the test enforces the API contract between
  // C++ and JS isn't violated.
  return FormatPrintfTemplate("pcsc_lite_client_handler_%d_call_function",
                              handler_id);
}

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

LONG ExtractReturnCodeAndResults(optional<Value> reply) {
  GOOGLE_SMART_CARD_CHECK(reply);
  GOOGLE_SMART_CARD_CHECK(reply->is_array());
  auto& reply_array = reply->GetArray();
  GOOGLE_SMART_CARD_CHECK(reply_array.size() == 1);
  LONG return_code = ConvertFromValueOrDie<LONG>(std::move(*reply_array[0]));
  return return_code;
}

template <typename Arg, typename... Args>
LONG ExtractReturnCodeAndResults(optional<Value> reply,
                                 Arg& out_arg,
                                 Args&... out_args) {
  GOOGLE_SMART_CARD_CHECK(reply);
  GOOGLE_SMART_CARD_CHECK(reply->is_array());
  auto& reply_array = reply->GetArray();
  if (reply_array.size() == 1) {
    // The reply contains only a return code - extract it.
    return ExtractReturnCodeAndResults(std::move(reply));
  }
  GOOGLE_SMART_CARD_CHECK(reply_array.size() >= 2);
  out_arg = ConvertFromValueOrDie<Arg>(std::move(*reply_array[1]));
  reply_array.erase(reply_array.begin() + 1);
  return ExtractReturnCodeAndResults(std::move(reply), out_args...);
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

  ~SmartCardConnectorApplicationTest() {
    application_->ShutDownAndWait();
#ifdef __native_client__
    EXPECT_TRUE(UnmountNaclIoFolders());
#endif  // __native_client__
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
    EXPECT_TRUE(pcsc_lite_ready_message_waiter->value()->StrictlyEquals(
        Value(Value::Type::kDictionary)));
  }

  // Enables the specified fake USB devices.
  void SetUsbDevices(
      const std::vector<TestingSmartCardSimulation::Device>& devices) {
    smart_card_simulation_.SetDevices(devices);
  }

  void SimulateFakeJsMessage(const std::string& message_type,
                             Value message_data) {
    TypedMessage typed_message;
    typed_message.type = message_type;
    typed_message.data = std::move(message_data);
    std::string error_message;
    if (!typed_message_router_.OnMessageReceived(
            ConvertToValueOrDie(std::move(typed_message)), &error_message)) {
      ADD_FAILURE() << "Failed handling fake JS message: " << error_message;
    }
  }

  ReaderNotificationObserver& reader_notification_observer() {
    return reader_notification_observer_;
  }

  // Sends a simulated JS-to-C++ notification of a PC/SC client being added (in
  // the real world it's usually another Chrome Extension that wants to access
  // smart cards).
  void SimulateJsClientAdded(int handler_id,
                             const std::string& client_name_for_log) {
    SimulateFakeJsMessage("pcsc_lite_create_client_handler",
                          DictValueBuilder()
                              .Add("handler_id", handler_id)
                              .Add("client_name_for_log", client_name_for_log)
                              .Get());
  }

  // Sends a simulated JS-to-C++ notification of a PC/SC client being removed.
  void SimulateJsClientRemoved(int handler_id) {
    SimulateFakeJsMessage(
        "pcsc_lite_delete_client_handler",
        DictValueBuilder().Add("handler_id", handler_id).Get());
  }

  // Sends a simulated JS-to-C++ request to call a PC/SC function.
  void SimulateCallFromJs(const std::string& requester_name,
                          RequestId request_id,
                          const std::string& function_name,
                          Value arguments) {
    RemoteCallRequestPayload remote_call_payload;
    remote_call_payload.function_name = function_name;
    // Convert an array `Value` to `vector<Value>`. Ideally the conversion
    // wouldn't be needed, but in tests it's more convenient pass a single
    // `Value` (e.g., constructed via `ArrayValueBuilder`).
    remote_call_payload.arguments =
        ConvertFromValueOrDie<std::vector<Value>>(std::move(arguments));

    RequestMessageData request_data;
    request_data.request_id = request_id;
    request_data.payload = ConvertToValueOrDie(std::move(remote_call_payload));

    SimulateFakeJsMessage(GetRequestMessageType(requester_name),
                          ConvertToValueOrDie(std::move(request_data)));
  }

  optional<Value> SimulateSyncCallFromJsClient(int handler_id,
                                               const std::string& function_name,
                                               Value arguments) {
    const RequestId request_id = ++request_id_counter_;
    const std::string requester_name = GetJsClientRequesterName(handler_id);
    auto waiter =
        global_context_.CreateResponseWaiter(requester_name, request_id);
    SimulateCallFromJs(requester_name, request_id, function_name,
                       std::move(arguments));
    waiter->Wait();
    return std::move(*waiter).take_value();
  }

  LONG SimulateEstablishContextCallFromJsClient(
      int handler_id,
      DWORD scope,
      Value reserved1,
      Value reserved2,
      SCARDCONTEXT& out_scard_context) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(handler_id,
                                     /*function_name=*/"SCardEstablishContext",
                                     ArrayValueBuilder()
                                         .Add(scope)
                                         .Add(std::move(reserved1))
                                         .Add(std::move(reserved2))
                                         .Get()),
        out_scard_context);
  }

  LONG SimulateReleaseContextCallFromJsClient(int handler_id,
                                              SCARDCONTEXT scard_context) {
    return ExtractReturnCodeAndResults(SimulateSyncCallFromJsClient(
        handler_id,
        /*function_name=*/"SCardReleaseContext",
        ArrayValueBuilder().Add(scard_context).Get()));
  }

  LONG SimulateListReadersCallFromJsClient(
      int handler_id,
      SCARDCONTEXT scard_context,
      Value groups,
      std::vector<std::string>& out_readers) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(handler_id,
                                     /*function_name=*/"SCardListReaders",
                                     ArrayValueBuilder()
                                         .Add(scard_context)
                                         .Add(std::move(groups))
                                         .Get()),
        out_readers);
  }

  LONG SimulateIsValidContextCallFromJsClient(int handler_id,
                                              SCARDCONTEXT scard_context) {
    return ExtractReturnCodeAndResults(SimulateSyncCallFromJsClient(
        handler_id,
        /*function_name=*/"SCardIsValidContext",
        ArrayValueBuilder().Add(scard_context).Get()));
  }

  LONG SimulateGetStatusChangeCallFromJsClient(
      int handler_id,
      SCARDCONTEXT scard_context,
      DWORD timeout,
      Value in_reader_states,
      std::vector<Value>& out_reader_states) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(handler_id,
                                     /*function_name=*/"SCardGetStatusChange",
                                     ArrayValueBuilder()
                                         .Add(scard_context)
                                         .Add(timeout)
                                         .Add(std::move(in_reader_states))
                                         .Get()),
        out_reader_states);
  }

  LONG SimulateConnectCallFromJsClient(int handler_id,
                                       SCARDCONTEXT scard_context,
                                       const std::string& reader_name,
                                       DWORD share_mode,
                                       DWORD preferred_protocols,
                                       SCARDHANDLE& out_scard_handle,
                                       DWORD& out_active_protocol) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(handler_id,
                                     /*function_name=*/"SCardConnect",
                                     ArrayValueBuilder()
                                         .Add(scard_context)
                                         .Add(reader_name)
                                         .Add(share_mode)
                                         .Add(preferred_protocols)
                                         .Get()),
        out_scard_handle, out_active_protocol);
  }

  LONG SimulateReconnectCallFromJsClient(int handler_id,
                                         SCARDHANDLE scard_handle,
                                         DWORD share_mode,
                                         DWORD preferred_protocols,
                                         DWORD initialization,
                                         DWORD& out_active_protocol) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(handler_id,
                                     /*function_name=*/"SCardReconnect",
                                     ArrayValueBuilder()
                                         .Add(scard_handle)
                                         .Add(share_mode)
                                         .Add(preferred_protocols)
                                         .Add(initialization)
                                         .Get()),
        out_active_protocol);
  }

  LONG SimulateDisconnectCallFromJsClient(int handler_id,
                                          SCARDHANDLE scard_handle,
                                          DWORD disposition) {
    return ExtractReturnCodeAndResults(SimulateSyncCallFromJsClient(
        handler_id,
        /*function_name=*/"SCardDisconnect",
        ArrayValueBuilder().Add(scard_handle).Add(disposition).Get()));
  }

  LONG SimulateTransmitCallFromJsClient(
      int handler_id,
      SCARDHANDLE scard_handle,
      DWORD send_protocol,
      const std::vector<uint8_t>& data_to_send,
      optional<DWORD> receive_protocol,
      DWORD& out_response_protocol,
      std::vector<uint8_t>& out_response) {
    Value receive_protocol_arg;
    if (receive_protocol) {
      receive_protocol_arg =
          DictValueBuilder().Add("protocol", *receive_protocol).Get();
    }
    Value response_protocol_information;
    LONG return_code = ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(
            handler_id,
            /*function_name=*/"SCardTransmit",
            ArrayValueBuilder()
                .Add(scard_handle)
                .Add(DictValueBuilder().Add("protocol", send_protocol).Get())
                .Add(data_to_send)
                .Add(std::move(receive_protocol_arg))
                .Get()),
        response_protocol_information, out_response);
    if (return_code == SCARD_S_SUCCESS) {
      out_response_protocol =
          response_protocol_information.GetDictionaryItem("protocol")
              ->GetInteger();
    }
    return return_code;
  }

  LONG SimulateBeginTransactionCallFromJsClient(int handler_id,
                                                SCARDHANDLE scard_handle) {
    return ExtractReturnCodeAndResults(SimulateSyncCallFromJsClient(
        handler_id,
        /*function_name=*/"SCardBeginTransaction",
        ArrayValueBuilder().Add(scard_handle).Get()));
  }

  LONG SimulateEndTransactionCallFromJsClient(int handler_id,
                                              SCARDHANDLE scard_handle,
                                              DWORD disposition) {
    return ExtractReturnCodeAndResults(SimulateSyncCallFromJsClient(
        handler_id,
        /*function_name=*/"SCardEndTransaction",
        ArrayValueBuilder().Add(scard_handle).Add(disposition).Get()));
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
  std::atomic_int request_id_counter_{0};
};

TEST_F(SmartCardConnectorApplicationTest, SmokeTest) {
  StartApplication();
}

// A PC/SC-Lite context can be established and freed, via direct C function
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

// A single reader is successfully initialized by PC/SC-Lite and is returned via
// the direct C function call `SCardListReaders()`.
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

  EXPECT_THAT(readers, ElementsAre(kGemaltoPcTwinReaderPcscName0));
}

// The direct C function call `SCardGetStatusChange()` detects when a reader is
// plugged in.
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
  reader_states[0].szReader = kPnpNotification;
  reader_states[0].dwCurrentState = SCARD_STATE_UNAWARE;
  EXPECT_EQ(SCardGetStatusChange(scard_context, /*dwTimeout=*/INFINITE,
                                 reader_states.data(), reader_states.size()),
            SCARD_S_SUCCESS);

  std::vector<std::string> readers = DirectCallSCardListReaders(scard_context);

  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);

  // Assert:

  EXPECT_EQ(reader_states[0].dwEventState,
            static_cast<DWORD>(SCARD_STATE_CHANGED));
  EXPECT_THAT(readers, ElementsAre(kGemaltoPcTwinReaderPcscName0));
  EXPECT_EQ(reader_notification_observer().WaitAndPop(),
            "reader_init_add:Gemalto PC Twin Reader");
  EXPECT_EQ(reader_notification_observer().WaitAndPop(),
            "reader_finish_add:Gemalto PC Twin Reader");
  EXPECT_TRUE(reader_notification_observer().Empty());
}

// The direct C function call `SCardGetStatusChange()` detects when a reader is
// unplugged.
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
              ElementsAre(kGemaltoPcTwinReaderPcscName0));

  // Simulate disconnecting the reader.
  SetUsbDevices({});

  // Wait until PC/SC reports a change in the list of readers.
  std::vector<SCARD_READERSTATE> reader_states(1);
  reader_states[0].szReader = kPnpNotification;
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

// One client can't use PC/SC contexts belonging to another client.
TEST_F(SmartCardConnectorApplicationTest, ContextsIsolation) {
  static constexpr int kFirstHandlerId = 1234;
  static constexpr int kSecondHandlerId = 321;

  // Arrange:
  StartApplication();
  SimulateJsClientAdded(kFirstHandlerId, /*client_name_for_log=*/"foo");
  SimulateJsClientAdded(kSecondHandlerId, /*client_name_for_log=*/"bar");
  SCARDCONTEXT first_scard_context = 0;
  EXPECT_EQ(SimulateEstablishContextCallFromJsClient(
                kFirstHandlerId, SCARD_SCOPE_SYSTEM,
                /*reserved1=*/Value(),
                /*reserved2=*/Value(), first_scard_context),
            SCARD_S_SUCCESS);
  SCARDCONTEXT second_scard_context = 0;
  EXPECT_EQ(SimulateEstablishContextCallFromJsClient(
                kSecondHandlerId, SCARD_SCOPE_SYSTEM,
                /*reserved1=*/Value(),
                /*reserved2=*/Value(), second_scard_context),
            SCARD_S_SUCCESS);
  EXPECT_NE(first_scard_context, second_scard_context);

  // Assert:
  EXPECT_EQ(SimulateIsValidContextCallFromJsClient(kFirstHandlerId,
                                                   second_scard_context),
            SCARD_E_INVALID_HANDLE);
  EXPECT_EQ(SimulateIsValidContextCallFromJsClient(kSecondHandlerId,
                                                   first_scard_context),
            SCARD_E_INVALID_HANDLE);
  EXPECT_EQ(SimulateReleaseContextCallFromJsClient(kFirstHandlerId,
                                                   second_scard_context),
            SCARD_E_INVALID_HANDLE);
  EXPECT_EQ(SimulateReleaseContextCallFromJsClient(kSecondHandlerId,
                                                   first_scard_context),
            SCARD_E_INVALID_HANDLE);

  // Cleanup:
  EXPECT_EQ(SimulateReleaseContextCallFromJsClient(kFirstHandlerId,
                                                   first_scard_context),
            SCARD_S_SUCCESS);
  EXPECT_EQ(SimulateReleaseContextCallFromJsClient(kSecondHandlerId,
                                                   second_scard_context),
            SCARD_S_SUCCESS);
  SimulateJsClientRemoved(kFirstHandlerId);
  SimulateJsClientRemoved(kSecondHandlerId);
}

// Test fixture that simplifies simulating commands from a single client
// application.
class SmartCardConnectorApplicationSingleClientTest
    : public SmartCardConnectorApplicationTest {
 protected:
  static constexpr int kFakeHandlerId = 1234;
  static constexpr const char* kFakeClientNameForLog =
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

  void TearDown() override {
    if (scard_context_)
      TearDownSCardContext();
    if (js_client_setup_)
      SimulateJsClientRemoved(kFakeHandlerId);

    SmartCardConnectorApplicationTest::TearDown();
  }

  void SetUpJsClient() {
    GOOGLE_SMART_CARD_CHECK(!js_client_setup_);
    SimulateJsClientAdded(kFakeHandlerId, kFakeClientNameForLog);
    js_client_setup_ = true;
  }

  void SetUpSCardContext() {
    GOOGLE_SMART_CARD_CHECK(!scard_context_);
    SCARDCONTEXT local_scard_context = 0;
    EXPECT_EQ(SimulateEstablishContextCallFromJsClient(
                  kFakeHandlerId, SCARD_SCOPE_SYSTEM,
                  /*reserved1=*/Value(),
                  /*reserved2=*/Value(), local_scard_context),
              SCARD_S_SUCCESS);
    scard_context_ = local_scard_context;
  }

  void TearDownSCardContext() {
    GOOGLE_SMART_CARD_CHECK(scard_context_);
    EXPECT_EQ(
        SimulateReleaseContextCallFromJsClient(kFakeHandlerId, *scard_context_),
        SCARD_S_SUCCESS);
    scard_context_ = {};
  }

  SCARDCONTEXT scard_context() const { return *scard_context_; }

 private:
  bool js_client_setup_ = false;
  optional<SCARDCONTEXT> scard_context_;
};

// `SCardEstablishContext()` and `SCardReleaseContext()` calls from JS succeed.
TEST_F(SmartCardConnectorApplicationSingleClientTest, SCardEstablishContext) {
  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  SetUpSCardContext();
  TearDownSCardContext();
}

// `SCardIsValidContext()` call from JS recognizes an existing context.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardIsValidContextCorrect) {
  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  SetUpSCardContext();
  LONG return_code =
      SimulateIsValidContextCallFromJsClient(kFakeHandlerId, scard_context());

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
}

// `SCardIsValidContext()` call from JS rejects a random value.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardIsValidContextWrong) {
  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  SetUpSCardContext();
  LONG return_code = SimulateIsValidContextCallFromJsClient(
      kFakeHandlerId, scard_context() + 1);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardIsValidContext()` call from JS rejects an already-released context.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardIsValidContextReleased) {
  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  SetUpSCardContext();
  SCARDCONTEXT cached_context = scard_context();
  TearDownSCardContext();
  LONG return_code =
      SimulateIsValidContextCallFromJsClient(kFakeHandlerId, cached_context);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardListReaders()` call from JS returns an error when there's no reader.
TEST_F(SmartCardConnectorApplicationSingleClientTest, SCardListReadersEmpty) {
  // Arrange:
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  std::vector<std::string> readers;
  LONG return_code =
      SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                          /*groups=*/Value(), readers);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_NO_READERS_AVAILABLE);
  EXPECT_THAT(readers, IsEmpty());
}

// `SCardListReaders()` call succeeds from JS when there's one device available.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardListReadersOneDevice) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  std::vector<std::string> readers;
  EXPECT_EQ(SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                                /*groups=*/Value(), readers),
            SCARD_S_SUCCESS);

  // Assert:
  EXPECT_THAT(readers, ElementsAre(kGemaltoPcTwinReaderPcscName0));
}

// `SCardGetStatusChange()` call from JS detects when a reader is plugged in.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeDeviceAppearing) {
  // Arrange:
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  // Simulate connecting a reader.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  // Request SCardGetStatusChange to check it observes the change.
  std::vector<Value> reader_states;
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kPnpNotification)
                             .Add("current_state", SCARD_STATE_UNAWARE)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);

  // Assert:
  ASSERT_THAT(reader_states, SizeIs(1));
  EXPECT_THAT(reader_states[0], DictSizeIs(4));
  EXPECT_THAT(reader_states[0], DictContains("reader_name", kPnpNotification));
  EXPECT_THAT(reader_states[0],
              DictContains("current_state", SCARD_STATE_UNAWARE));
  EXPECT_THAT(reader_states[0],
              DictContains("event_state", SCARD_STATE_CHANGED));
  EXPECT_THAT(reader_states[0], DictContains("atr", Value::Type::kBinary));
}

// `SCardGetStatusChange()` call from JS detects when a reader is unplugged.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeDeviceRemoving) {
  // Arrange: start with a single device.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  // Simulate disconnecting a reader.
  SetUsbDevices({});
  // Request SCardGetStatusChange to check it observes the change.
  std::vector<Value> reader_states;
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);

  // Assert:
  ASSERT_THAT(reader_states, SizeIs(1));
  ASSERT_THAT(reader_states[0], DictSizeIs(4));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  EXPECT_THAT(reader_states[0],
              DictContains("current_state", SCARD_STATE_EMPTY));
  EXPECT_THAT(reader_states[0], DictContains("atr", Value::Type::kBinary));
  // Depending on the timing, PC/SC may or may not report the
  // `SCARD_STATE_UNKNOWN` flag (this depends on whether it already removed the
  // "dead" reader from internal lists by the time SCardGetStatusChange is
  // replied to).
  const Value* const received_event_state =
      reader_states[0].GetDictionaryItem("event_state");
  ASSERT_TRUE(received_event_state);
  ASSERT_TRUE(received_event_state->is_integer());
  EXPECT_THAT(
      received_event_state->GetInteger(),
      AnyOf(SCARD_STATE_CHANGED | SCARD_STATE_UNKNOWN | SCARD_STATE_UNAVAILABLE,
            SCARD_STATE_CHANGED | SCARD_STATE_UNAVAILABLE));
}

// Test `SCardGetStatusChange()` call from JS returns the reader and card
// information.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeWithCard) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  std::vector<Value> reader_states;
  // This call is expected to return immediately, since we pass
  // `SCARD_STATE_UNKNOWN`.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_UNKNOWN)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);

  // Assert:
  ASSERT_THAT(reader_states, SizeIs(1));
  EXPECT_THAT(reader_states[0], DictSizeIs(4));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  EXPECT_THAT(reader_states[0],
              DictContains("current_state", SCARD_STATE_UNKNOWN));
  EXPECT_THAT(
      reader_states[0],
      DictContains("event_state", SCARD_STATE_CHANGED | SCARD_STATE_PRESENT));
  EXPECT_THAT(
      reader_states[0],
      DictContains("atr",
                   TestingSmartCardSimulation::GetCardAtr(
                       TestingSmartCardSimulation::CardType::kCosmoId70)));
}

// `SCardGetStatusChange()` call from JS detects when a card is inserted.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeCardInserting) {
  // Arrange: start without a card.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act: simulate the card insertion.
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  // Request SCardGetStatusChange to check it observes the change.
  std::vector<Value> reader_states;
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);

  // Assert:
  ASSERT_THAT(reader_states, SizeIs(1));
  EXPECT_THAT(reader_states[0], DictSizeIs(4));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  EXPECT_THAT(reader_states[0],
              DictContains("current_state", SCARD_STATE_EMPTY));
  // The "event_state" field contains the number of card insertion/removal
  // events in the higher 16 bits.
  EXPECT_THAT(reader_states[0],
              DictContains("event_state", SCARD_STATE_CHANGED |
                                              SCARD_STATE_PRESENT | 0x10000));
  EXPECT_THAT(
      reader_states[0],
      DictContains("atr",
                   TestingSmartCardSimulation::GetCardAtr(
                       TestingSmartCardSimulation::CardType::kCosmoId70)));
}

// `SCardConnect()` call from JS fails when there's no card inserted.
TEST_F(SmartCardConnectorApplicationSingleClientTest, SCardConnectErrorNoCard) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, scard_handle,
                active_protocol),
            SCARD_E_NO_SMARTCARD);
}

// `SCardConnect()` call from JS succeeds for dwShareMode `SCARD_SHARE_DIRECT`
// even when there's no card inserted.
TEST_F(SmartCardConnectorApplicationSingleClientTest, SCardConnectDirect) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_DIRECT,
                /*preferred_protocols=*/0, scard_handle, active_protocol),
            SCARD_S_SUCCESS);

  // Assert:
  EXPECT_NE(scard_handle, 0);
  EXPECT_EQ(active_protocol, 0U);

  // Cleanup:
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// `SCardConnect()` call from JS successfully connects to a card using the "T1"
// protocol.
TEST_F(SmartCardConnectorApplicationSingleClientTest, SCardConnectT1) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);

  // Assert:
  EXPECT_NE(scard_handle, 0);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));

  // Cleanup:
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// `SCardConnect()` call from JS fails to connect via the "T1" protocol if the
// previous connection was using the "RAW" protocol.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardConnectProtocolMismatch) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Simulate an empty UpdateAdminPolicy message to unblock the WaitAndGet()
  // call. This is normally sent when admin-policy-service.js is first
  // initialized.
  SimulateFakeJsMessage("update_admin_policy", {});

  // Act:
  // Connect via the "RAW" protocol and disconnect.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_RAW, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_NE(scard_handle, 0);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_RAW));
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
  // Attempt connecting via a different protocol ("ANY" denotes "either T0 or
  // T1").
  LONG return_code = SimulateConnectCallFromJsClient(
      kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
      SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, scard_handle, active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_PROTO_MISMATCH);
}

// If the client is allowed to use the SCardDisconnect fallback by admin policy,
// `SCardConnect()` call from JS succeeds to connect via the "T1" protocol
// even if the previous connection was using the "RAW" protocol.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardConnectProtocolMismatchDisconnectFallback) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Simulate an UpdateAdminPolicy message to allowlist the client.
  SimulateFakeJsMessage(
      "update_admin_policy",
      DictValueBuilder()
          .Add("scard_disconnect_fallback_client_app_ids",
               ArrayValueBuilder().Add(kFakeClientNameForLog).Get())
          .Get());

  // Act:
  // Connect via the "RAW" protocol and disconnect.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_RAW, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_NE(scard_handle, 0);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_RAW));
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
  // Attempt connecting via a different protocol ("ANY" denotes "either T0 or
  // T1").
  LONG return_code = SimulateConnectCallFromJsClient(
      kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
      SCARD_SHARE_SHARED, /*preferred_protocols=*/SCARD_PROTOCOL_ANY,
      scard_handle, active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));
}

// `SCardConnect()` call from JS successfully connects via the "T1" protocol if
// the previous connection via the "RAW" protocol was terminated by
// `SCardDisconnect` with `SCARD_RESET_CARD`.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardConnectProtocolChangeAfterReset) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  // Connect via the "RAW" protocol and disconnect with resetting the card.
  SCARDHANDLE first_scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_RAW, first_scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_NE(first_scard_handle, 0);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_RAW));
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(
                kFakeHandlerId, first_scard_handle, SCARD_RESET_CARD),
            SCARD_S_SUCCESS);
  // Attempt connecting via a different protocol ("ANY" denotes "either T0 or
  // T1").
  SCARDHANDLE second_scard_handle = 0;
  LONG return_code = SimulateConnectCallFromJsClient(
      kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
      SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, second_scard_handle,
      active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));
  EXPECT_NE(second_scard_handle, first_scard_handle);

  // Cleanup:
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(
                kFakeHandlerId, second_scard_handle, SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// `SCardReconnect()` call from JS succeeds when using the same parameters as
// the previous `SCardConnect()` call.
TEST_F(SmartCardConnectorApplicationSingleClientTest, SCardReconnect) {
  // Arrange:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Connect to the card.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_NE(scard_handle, 0);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));

  // Act:
  // Reconnect using the same sharing and protocol.
  DWORD new_active_protocol = 0;
  LONG return_code = SimulateReconnectCallFromJsClient(
      kFakeHandlerId, scard_handle, SCARD_SHARE_SHARED,
      /*preferred_protocols=*/SCARD_PROTOCOL_ANY, SCARD_LEAVE_CARD,
      new_active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(new_active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));

  // Cleanup:
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// Calling a non-existing PC/SC function results in an error (but not crash).
TEST_F(SmartCardConnectorApplicationSingleClientTest, NonExistingFunctionCall) {
  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  optional<Value> response =
      SimulateSyncCallFromJsClient(kFakeHandlerId,
                                   /*function_name=*/"foo",
                                   /*arguments=*/Value(Value::Type::kArray));

  // Assert: the response is null as it only contains an error message (we don't
  // verify the message here).
  EXPECT_THAT(response, IsNullOptional());
}

// `SCardDisconnect()` and `SCardReleaseContext()` calls from JS should succeed
// even after the reader disappeared when there was an active card handle. This
// is a regression test for a PC/SC-Lite bug (see
// <https://github.com/GoogleChromeLabs/chromeos_smart_card_connector/issues/681>).
TEST_F(SmartCardConnectorApplicationSingleClientTest, DisconnectAfterRemoving) {
  // Arrange. Start with a reader and a card available.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Connect to the card.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_ANY, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);

  // Act. Simulate disconnecting the reader.
  SetUsbDevices({});
  // Wait until PC/SC-Lite reports the change in the reader list.
  std::vector<SCARD_READERSTATE> reader_states(1);
  reader_states[0].szReader = kPnpNotification;
  reader_states[0].dwCurrentState = SCARD_STATE_UNAWARE;
  EXPECT_EQ(SCardGetStatusChange(scard_context(), /*dwTimeout=*/INFINITE,
                                 reader_states.data(), reader_states.size()),
            SCARD_S_SUCCESS);
  // Try disconnecting the card handle.
  LONG return_code = SimulateDisconnectCallFromJsClient(
      kFakeHandlerId, scard_handle, SCARD_LEAVE_CARD);

  // Assert.
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  // Note: `SCardReleaseContext()` is called and its result is verified by the
  // fixture.
}

// `SCardTransmit()` calls from JS should be able to send a request APDU to the
// card and receive a response. We use a fake PIV card in this test.
TEST_F(SmartCardConnectorApplicationSingleClientTest, TransmitPivCommands) {
  // Arrange: set up a reader and a card with a PIV profile.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  device.card_profile =
      TestingSmartCardSimulation::CardProfile::kCharismathicsPiv;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Connect to the card.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_T1, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);

  // Act:
  {
    // Send the SELECT command (the format is per NIST 800-73-4).
    const std::vector<uint8_t> kSelectCommand = {0x00, 0xA4, 0x04, 0x00, 0x09,
                                                 0xA0, 0x00, 0x00, 0x03, 0x08,
                                                 0x00, 0x00, 0x10, 0x00, 0x00};
    std::vector<uint8_t> response;
    DWORD response_protocol = 0;
    EXPECT_EQ(
        SimulateTransmitCallFromJsClient(
            kFakeHandlerId, scard_handle, SCARD_PROTOCOL_T1, kSelectCommand,
            /*receive_protocol=*/{}, response_protocol, response),
        SCARD_S_SUCCESS);
    EXPECT_EQ(response_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));
    // The expected result should contain the application identifier followed by
    // 0x90 0x00 (denoting a successful operation).
    std::vector<uint8_t> expected_response =
        TestingSmartCardSimulation::GetCardProfileApplicationIdentifier(
            TestingSmartCardSimulation::CardProfile::kCharismathicsPiv);
    expected_response.push_back(0x90);
    expected_response.push_back(0x00);
    EXPECT_EQ(response, expected_response);
  }

  // Cleanup:
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// `SCardBeginTransaction()` calls from JS should succeed.
TEST_F(SmartCardConnectorApplicationSingleClientTest, BeginTransaction) {
  // Arrange: set up a reader and a card with a PIV profile.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Connect to the card.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_T1, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);

  // Act:
  LONG return_code =
      SimulateBeginTransactionCallFromJsClient(kFakeHandlerId, scard_handle);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);

  // Cleanup. Note that we also verify here the transaction gets ended
  // automatically.
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// `SCardEndTransaction()` calls from JS should succeed.
TEST_F(SmartCardConnectorApplicationSingleClientTest, EndTransaction) {
  // Arrange: set up a reader and a card with a PIV profile.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Connect to the card and begin a transaction.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_T1, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_EQ(
      SimulateBeginTransactionCallFromJsClient(kFakeHandlerId, scard_handle),
      SCARD_S_SUCCESS);

  // Act:
  LONG return_code = SimulateEndTransactionCallFromJsClient(
      kFakeHandlerId, scard_handle, SCARD_LEAVE_CARD);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);

  // Cleanup:
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

}  // namespace google_smart_card
