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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <limits>
#include <locale>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <reader.h>
#include <winscard.h>

#include "common/cpp/src/public/formatting.h"
#include "common/cpp/src/public/messaging/typed_message.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/multi_string.h"
#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/optional_test_utils.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/requesting/requester_message.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/cpp/src/public/value_test_utils.h"

#include "common/cpp/src/public/testing_global_context.h"
#include "common/cpp/src/public/value_builder.h"
#include "smart_card_connector_app/src/testing_smart_card_simulation.h"
#include "third_party/libusb/webport/src/libusb_js_proxy_constants.h"

#ifdef __native_client__
#include "common/cpp/src/public/nacl_io_utils.h"
#endif  // __native_client__

#ifdef __native_client__
// Native Client's version of Google Test uses a different name of the macro for
// parameterized tests.
#define INSTANTIATE_TEST_SUITE_P INSTANTIATE_TEST_CASE_P
// Native Client's version of Google Test macro INSTANTIATE_TEST_CASE_P
// produces this warning when being used without test generator parameters.
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif  // __native_client__

using testing::AnyOf;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;

namespace google_smart_card {

namespace {

// The upper boundary on how long the `Application` shutdown may take. Used
// primarily to catch mistakes when the shutdown is hung completely or is
// wrongly blocked on multi-minute timeouts.
constexpr auto kApplicationShutdownTimeLimit = std::chrono::minutes(1);

// The constant from the PC/SC-Lite API docs.
constexpr char kPnpNotification[] = R"(\\?PnP?\Notification)";
// Names of `TestingSmartCardSimulation::DeviceType` items as they appear in the
// PC/SC-Lite API. The "0" suffix corresponds to the "00 00" part that contains
// nonzeroes in case there are multiple devices.
constexpr char kGemaltoPcTwinReaderPcscName0[] = "Gemalto PC Twin Reader 00 00";
constexpr char kDellSmartCardReaderKeyboardPcscName0[] =
    "Dell Dell Smart Card Reader Keyboard 00 00";
constexpr char kDellSmartCardReaderKeyboardPcscName1[] =
    "Dell Dell Smart Card Reader Keyboard 01 00";
constexpr char kYubikey4CPcscName0[] = "Yubico YubiKey CCID 00 00";
// Corresponds to the `TAG_IFD_DEVICE_REMOVED` constant in the PC/SC
// implementation.
constexpr int kTagIfdDeviceRemoved = 0x0FB4;

MATCHER_P(HasBitsSet,
          bits,
          /*description_string=*/std::string("has set bits ") +
              ::testing::PrintToString(bits)) {
  return (arg & bits) == bits;
}

// Records reader_* messages sent to JS and allows to inspect them in tests.
class ReaderNotificationObserver final {
 public:
  void Init(TestingGlobalContext& global_context) {
    for (const auto* event_name :
         {"reader_init_add", "reader_finish_add", "reader_remove"}) {
      global_context.RegisterMessageHandler(
          event_name,
          std::bind(&ReaderNotificationObserver::OnMessageToJs, this,
                    event_name, /*message_data=*/std::placeholders::_1));
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
  void OnMessageToJs(const std::string& event_name, Value message_data) {
    const Value* reader_name = message_data.GetDictionaryItem("readerName");
    GOOGLE_SMART_CARD_CHECK(reader_name);
    const Value* return_code = message_data.GetDictionaryItem("returnCode");

    std::string notification = event_name + ":" + reader_name->GetString();
    if (return_code && return_code->GetInteger() != 0)
      notification += ":error";

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

// Waits until the given functor returns true. The implementation is a simple
// periodic polling.
void WaitUntilPredicate(std::function<bool()> predicate) {
  const auto kPollingInterval = std::chrono::milliseconds(1);
  while (!predicate())
    std::this_thread::sleep_for(kPollingInterval);
}

template <typename Rep, typename Period>
void AssertDurationNotLonger(std::function<void()> f,
                             std::chrono::duration<Rep, Period> max_duration) {
  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();
  f();
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  ASSERT_LE(end - begin, max_duration);
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
    AssertDurationNotLonger([this] { application_->ShutDownAndWait(); },
                            kApplicationShutdownTimeLimit);

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

  std::string SimulateStringifyErrorCallFromJsClient(int handler_id,
                                                     LONG error) {
    optional<Value> reply =
        SimulateSyncCallFromJsClient(handler_id,
                                     /*function_name=*/"pcsc_stringify_error",
                                     ArrayValueBuilder().Add(error).Get());
    // Extract the result manually because, unlike all other PC/SC functions,
    // the `pcsc_stringify_error()` method doesn't return error codes.
    GOOGLE_SMART_CARD_CHECK(reply);
    GOOGLE_SMART_CARD_CHECK(reply->is_array());
    auto& reply_array = reply->GetArray();
    GOOGLE_SMART_CARD_CHECK(reply_array.size() == 1);
    return ConvertFromValueOrDie<std::string>(std::move(*reply_array[0]));
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

  LONG SimulateCancelCallFromJsClient(int handler_id,
                                      SCARDCONTEXT scard_context) {
    return ExtractReturnCodeAndResults(SimulateSyncCallFromJsClient(
        handler_id,
        /*function_name=*/"SCardCancel",
        ArrayValueBuilder().Add(scard_context).Get()));
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

  LONG SimulateStatusCallFromJsClient(int handler_id,
                                      SCARDHANDLE scard_handle,
                                      std::string& out_reader_name,
                                      DWORD& out_state,
                                      DWORD& out_protocol,
                                      std::vector<uint8_t>& out_atr) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(
            handler_id,
            /*function_name=*/"SCardStatus",
            ArrayValueBuilder().Add(scard_handle).Get()),
        out_reader_name, out_state, out_protocol, out_atr);
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

  LONG SimulateGetAttribCallFromJsClient(int handler_id,
                                         SCARDHANDLE scard_handle,
                                         DWORD attr_id,
                                         std::vector<uint8_t>& out_attr) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(
            handler_id,
            /*function_name=*/"SCardGetAttrib",
            ArrayValueBuilder().Add(scard_handle).Add(attr_id).Get()),
        out_attr);
  }

  LONG SimulateSetAttribCallFromJsClient(int handler_id,
                                         SCARDHANDLE scard_handle,
                                         DWORD attr_id,
                                         const std::vector<uint8_t>& attr) {
    return ExtractReturnCodeAndResults(SimulateSyncCallFromJsClient(
        handler_id,
        /*function_name=*/"SCardSetAttrib",
        ArrayValueBuilder().Add(scard_handle).Add(attr_id).Add(attr).Get()));
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

  LONG SimulateControlCallFromJsClient(int handler_id,
                                       SCARDHANDLE scard_handle,
                                       DWORD control_code,
                                       const std::vector<uint8_t>& request_data,
                                       std::vector<uint8_t>& out_response) {
    return ExtractReturnCodeAndResults(
        SimulateSyncCallFromJsClient(handler_id,
                                     /*function_name=*/"SCardControl",
                                     ArrayValueBuilder()
                                         .Add(scard_handle)
                                         .Add(control_code)
                                         .Add(request_data)
                                         .Get()),
        out_response);
  }

 private:
  void SetUpUsbSimulation() {
    global_context_.RegisterRequestRerouter(
        /*original_requester_name=*/kLibusbJsProxyRequesterName,
        /*new_requester_name=*/TestingSmartCardSimulation::kRequesterName);
  }

  TypedMessageRouter typed_message_router_;
  TestingGlobalContext global_context_{&typed_message_router_};
  TestingSmartCardSimulation smart_card_simulation_{&global_context_,
                                                    &typed_message_router_};
  ReaderNotificationObserver reader_notification_observer_;
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

// Test that after a client is removed, the context it opened eventually becomes
// released.
TEST_F(SmartCardConnectorApplicationTest, AutoCleanupContext) {
  constexpr int kHandlerId = 1234;

  // Arrange:
  StartApplication();
  SimulateJsClientAdded(kHandlerId, /*client_name_for_log=*/"foo");
  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SimulateEstablishContextCallFromJsClient(
                kHandlerId, SCARD_SCOPE_SYSTEM,
                /*reserved1=*/Value(),
                /*reserved2=*/Value(), scard_context),
            SCARD_S_SUCCESS);
  EXPECT_EQ(SCardIsValidContext(scard_context), SCARD_S_SUCCESS);

  // Act:
  SimulateJsClientRemoved(kHandlerId);

  // Assert: the context should eventually become invalid (as it's freed by a
  // background thread there's no easy way to observe this without polling).
  WaitUntilPredicate([&]() {
    return SCardIsValidContext(scard_context) == SCARD_E_INVALID_HANDLE;
  });
}

// Regression test for the shutdown crashes in case there's an active JS client.
TEST_F(SmartCardConnectorApplicationTest, ShutdownWithActiveClient) {
  constexpr int kHandlerId = 1234;

  // Arrange:
  StartApplication();
  SimulateJsClientAdded(kHandlerId, /*client_name_for_log=*/"foo");
  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SimulateEstablishContextCallFromJsClient(
                kHandlerId, SCARD_SCOPE_SYSTEM,
                /*reserved1=*/Value(),
                /*reserved2=*/Value(), scard_context),
            SCARD_S_SUCCESS);

  // No act/assert sections, since we just want to check the teardown doesn't
  // crash.
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

// `SCardReleaseContext()` call from JS fails on a wrong context when there's no
// established contexts at all.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardReleaseContextErrorNone) {
  const SCARDCONTEXT kBadContext = 12345;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  const LONG return_code =
      SimulateReleaseContextCallFromJsClient(kFakeHandlerId, kBadContext);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardReleaseContext()` call from JS fails on a wrong context when there's
// another established context.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardReleaseContextErrorDifferent) {
  // Arrange:
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  const SCARDCONTEXT bad_context = scard_context() ^ 1;
  const LONG return_code =
      SimulateReleaseContextCallFromJsClient(kFakeHandlerId, bad_context);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardReleaseContext()` call from JS fails to release a context more than
// once.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardReleaseContextErrorAlreadyReleased) {
  // Arrange. Remember the established context `scard_context_` before it's
  // released and this variable is cleared.
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  const SCARDCONTEXT local_scard_context = scard_context();
  TearDownSCardContext();

  // Act:
  const LONG return_code = SimulateReleaseContextCallFromJsClient(
      kFakeHandlerId, local_scard_context);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

MATCHER(IsPrintableNonEmptyString, "") {
  if (arg.empty())
    return false;
#ifdef __native_client__
  // Constructing a locale object causes a crash under NaCl, hence we use the C
  // library function when on NaCl toolchain.
  return std::all_of(arg.begin(), arg.end(), [](char c) { return isprint(c); });
#else
  std::locale c_locale("C");
  return std::all_of(arg.begin(), arg.end(),
                     [&](char c) { return std::isprint(c, c_locale); });
#endif
}

// `pcsc_stringify_error()` calls from JS succeed with reasonable results for
// each possible error code.
// We don't check against golden strings because hardcoding them all in the test
// would make little sense.
TEST_F(SmartCardConnectorApplicationSingleClientTest, StringifyError) {
  constexpr LONG kFirstError = SCARD_F_INTERNAL_ERROR;
  constexpr LONG kLastError = SCARD_W_CARD_NOT_AUTHENTICATED;
  constexpr LONG kNonExistingError = 1;
  constexpr LONG kMinValue = std::numeric_limits<LONG>::min();
  constexpr LONG kMaxValue = std::numeric_limits<LONG>::max();

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  // Check the successful return code. It's a special case because its numerical
  // value (0) is distant from all error codes.
  EXPECT_THAT(
      SimulateStringifyErrorCallFromJsClient(kFakeHandlerId, SCARD_S_SUCCESS),
      IsPrintableNonEmptyString());
  // Try every value within the range of known error codes (there are gaps
  // within this range, but the code-under-test should handle them as well).
  for (LONG code = std::min(kFirstError, kLastError);
       code <= std::max(kFirstError, kLastError); ++code) {
    EXPECT_THAT(SimulateStringifyErrorCallFromJsClient(kFakeHandlerId, code),
                IsPrintableNonEmptyString())
        << code;
  }
  // Try explicitly unknown and extreme values. The code-under-test should
  // return reasonable results for them too.
  for (LONG code : {kNonExistingError, kMinValue, kMaxValue}) {
    EXPECT_THAT(SimulateStringifyErrorCallFromJsClient(kFakeHandlerId, code),
                IsPrintableNonEmptyString())
        << code;
  }
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

// `SCardListReaders()` call from JS fails when using a wrong context.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardListReadersWrongContext) {
  constexpr SCARDCONTEXT kWrongScardContext = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  std::vector<std::string> readers;
  LONG return_code =
      SimulateListReadersCallFromJsClient(kFakeHandlerId, kWrongScardContext,
                                          /*groups=*/Value(), readers);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
  EXPECT_THAT(readers, IsEmpty());
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

// Same as the SCardGetStatusChangeCardInserting test above, however the card is
// inserted a bit later than when the `SCardGetStatusChange()` call starts. The
// visible behavior should be the same, however the test might exercise
// different implementation codepaths.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeCardInsertingDelayed) {
  // Arrange: start with a single empty reader.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Start a blocking `SCardGetStatusChange()` call on a different thread.
  std::vector<Value> reader_states;
  std::future<LONG> pending_return_code = std::async(std::launch::async, [&] {
    return SimulateGetStatusChangeCallFromJsClient(
        kFakeHandlerId, scard_context(),
        /*timeout=*/INFINITE,
        ArrayValueBuilder()
            .Add(DictValueBuilder()
                     .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                     .Add("current_state", SCARD_STATE_EMPTY)
                     .Get())
            .Get(),
        reader_states);
  });
  // Check that the call is actually blocked. The exact interval isn't important
  // here - we just want some reasonably big probability of catching a bug if
  // it's introduced.
  EXPECT_EQ(pending_return_code.wait_for(std::chrono::seconds(1)),
            std::future_status::timeout);

  // Act: simulate the card insertion.
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  // Wait until the `SCardGetStatusChange()` call completes.
  const LONG return_code = pending_return_code.get();

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(reader_states[0],
              DictContains("event_state", SCARD_STATE_CHANGED |
                                              SCARD_STATE_PRESENT | 0x10000));
  EXPECT_THAT(
      reader_states[0],
      DictContains("atr",
                   TestingSmartCardSimulation::GetCardAtr(
                       TestingSmartCardSimulation::CardType::kCosmoId70)));
}

// `SCardGetStatusChange()` call from JS detects when a card is removed.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeCardRemoving) {
  // Arrange: start with a card.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act: simulate the card removal.
  device.card_type = {};
  SetUsbDevices({device});
  // Request SCardGetStatusChange to check it observes the change.
  std::vector<Value> reader_states;
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_PRESENT)
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
              DictContains("current_state", SCARD_STATE_PRESENT));
  // The "event_state" field contains the number of card insertion/removal
  // events in the higher 16 bits.
  EXPECT_THAT(reader_states[0],
              DictContains("event_state",
                           SCARD_STATE_CHANGED | SCARD_STATE_EMPTY | 0x10000));
  EXPECT_THAT(reader_states[0], DictContains("atr", std::vector<uint8_t>()));
}

// Same as the SCardGetStatusChangeCardRemoving test above, however the card is
// removed a bit later than when the `SCardGetStatusChange()` call starts. The
// visible behavior should be the same, however the test might exercise
// different implementation codepaths.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeCardRemovingDelayed) {
  // Arrange: start with a card.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Start a blocking `SCardGetStatusChange()` call on a different thread.
  std::vector<Value> reader_states;
  std::future<LONG> pending_return_code = std::async(std::launch::async, [&] {
    return SimulateGetStatusChangeCallFromJsClient(
        kFakeHandlerId, scard_context(),
        /*timeout=*/INFINITE,
        ArrayValueBuilder()
            .Add(DictValueBuilder()
                     .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                     .Add("current_state", SCARD_STATE_PRESENT)
                     .Get())
            .Get(),
        reader_states);
  });
  // Check that the call is actually blocked. The exact interval isn't important
  // here - we just want some reasonably big probability of catching a bug if
  // it's introduced.
  EXPECT_EQ(pending_return_code.wait_for(std::chrono::seconds(1)),
            std::future_status::timeout);

  // Act: simulate the card removal.
  device.card_type = {};
  SetUsbDevices({device});
  // Wait until the `SCardGetStatusChange()` call completes.
  const LONG return_code = pending_return_code.get();

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(reader_states[0],
              DictContains("event_state",
                           SCARD_STATE_CHANGED | SCARD_STATE_EMPTY | 0x10000));
  EXPECT_THAT(reader_states[0], DictContains("atr", std::vector<uint8_t>()));
}

// `SCardGetStatusChange()` call from JS correctly detects changes when there
// are multiple readers attached simultaneously.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeMultipleReaders) {
  // Arrange: start from a single reader without a card.
  TestingSmartCardSimulation::Device first_device;
  first_device.id = 123;
  first_device.type =
      TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({first_device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act: simulate plugging in another reader without a card.
  TestingSmartCardSimulation::Device second_device;
  second_device.id = 145;
  second_device.type =
      TestingSmartCardSimulation::DeviceType::kDellSmartCardReaderKeyboard;
  SetUsbDevices({first_device, second_device});
  // Request SCardGetStatusChange to check it observes the change in the list of
  // readers, but no change for the first reader.
  std::vector<Value> reader_states;
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name", kPnpNotification)
                             .Add("current_state", SCARD_STATE_UNAWARE)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  ASSERT_THAT(reader_states, SizeIs(2));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  EXPECT_THAT(reader_states[0], DictContains("event_state", SCARD_STATE_EMPTY));
  EXPECT_THAT(reader_states[1], DictContains("reader_name", kPnpNotification));
  EXPECT_THAT(reader_states[1],
              DictContains("event_state", SCARD_STATE_CHANGED));

  // Simulate inserting a card into the second reader.
  second_device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({first_device, second_device});
  // Request SCardGetStatusChange to check it observes the card insertion for
  // the second reader, but no changes for the first reader or for the list of
  // readers.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name",
                                  kDellSmartCardReaderKeyboardPcscName1)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name", kPnpNotification)
                             .Add("current_state", SCARD_STATE_UNAWARE)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  ASSERT_THAT(reader_states, SizeIs(3));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  EXPECT_THAT(reader_states[0], DictContains("event_state", SCARD_STATE_EMPTY));
  EXPECT_THAT(
      reader_states[1],
      DictContains("reader_name", kDellSmartCardReaderKeyboardPcscName1));
  // Depending on the timing, PC/SC may or may not add the 0x10000 event counter
  // (this depends on whether the internal monitoring thread got initialized
  // before or after the card insertion).
  EXPECT_THAT(
      reader_states[1],
      AnyOf(DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_PRESENT | 0x10000),
            DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_PRESENT)));
  EXPECT_THAT(reader_states[2], DictContains("reader_name", kPnpNotification));
  EXPECT_THAT(reader_states[2],
              DictContains("event_state", SCARD_STATE_UNAWARE));

  // Simulate inserting a card into the first reader.
  first_device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({first_device, second_device});
  // Request SCardGetStatusChange to check it observes the card insertion for
  // the first reader, but no changes for the second reader or for the list of
  // readers.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name",
                                  kDellSmartCardReaderKeyboardPcscName1)
                             .Add("current_state", SCARD_STATE_PRESENT)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name", kPnpNotification)
                             .Add("current_state", SCARD_STATE_UNAWARE)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  ASSERT_THAT(reader_states, SizeIs(3));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  // Unlike above, due to the ordering of the reader events we expect the event
  // counter to be incremented.
  EXPECT_THAT(reader_states[0],
              DictContains("event_state", SCARD_STATE_CHANGED |
                                              SCARD_STATE_PRESENT | 0x10000));
  EXPECT_THAT(
      reader_states[1],
      DictContains("reader_name", kDellSmartCardReaderKeyboardPcscName1));
  EXPECT_THAT(reader_states[1],
              AnyOf(DictContains("event_state", SCARD_STATE_PRESENT | 0x10000),
                    DictContains("event_state", SCARD_STATE_PRESENT)));
  EXPECT_THAT(reader_states[2], DictContains("reader_name", kPnpNotification));
  EXPECT_THAT(reader_states[2],
              DictContains("event_state", SCARD_STATE_UNAWARE));

  // Simulate removing a card from the second reader.
  second_device.card_type = {};
  SetUsbDevices({first_device, second_device});
  // Request SCardGetStatusChange to check it observes the card removal for the
  // second reader, but no changes for the first reader or for the list of
  // readers.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_PRESENT)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name",
                                  kDellSmartCardReaderKeyboardPcscName1)
                             .Add("current_state", SCARD_STATE_PRESENT)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name", kPnpNotification)
                             .Add("current_state", SCARD_STATE_UNAWARE)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  ASSERT_THAT(reader_states, SizeIs(3));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  EXPECT_THAT(reader_states[0],
              DictContains("event_state", SCARD_STATE_PRESENT | 0x10000));
  EXPECT_THAT(
      reader_states[1],
      DictContains("reader_name", kDellSmartCardReaderKeyboardPcscName1));
  // The event counter should get incremented now, becoming either 1 or 2.
  EXPECT_THAT(
      reader_states[1],
      AnyOf(DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_EMPTY | 0x20000),
            DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_EMPTY | 0x10000)));
  EXPECT_THAT(reader_states[2], DictContains("reader_name", kPnpNotification));
  EXPECT_THAT(reader_states[2],
              DictContains("event_state", SCARD_STATE_UNAWARE));

  // Simulate unplugging the first reader.
  SetUsbDevices({second_device});
  // Request SCardGetStatusChange to check it observes the first reader removal:
  // the reader's state should be reported as "unavailable", and the change in
  // the list of readers should be reported as well.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                             .Add("current_state", SCARD_STATE_PRESENT)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name",
                                  kDellSmartCardReaderKeyboardPcscName1)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Add(DictValueBuilder()
                             .Add("reader_name", kPnpNotification)
                             .Add("current_state", SCARD_STATE_UNAWARE)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  ASSERT_THAT(reader_states, SizeIs(3));
  EXPECT_THAT(reader_states[0],
              DictContains("reader_name", kGemaltoPcTwinReaderPcscName0));
  EXPECT_THAT(
      reader_states[0],
      DictContains("event_state", SCARD_STATE_CHANGED | SCARD_STATE_UNKNOWN |
                                      SCARD_STATE_UNAVAILABLE));
  EXPECT_THAT(
      reader_states[1],
      DictContains("reader_name", kDellSmartCardReaderKeyboardPcscName1));
  EXPECT_THAT(reader_states[1],
              AnyOf(DictContains("event_state", SCARD_STATE_EMPTY | 0x20000),
                    DictContains("event_state", SCARD_STATE_EMPTY | 0x10000)));
  EXPECT_THAT(reader_states[2], DictContains("reader_name", kPnpNotification));
  EXPECT_THAT(reader_states[2],
              DictContains("event_state", SCARD_STATE_CHANGED));
}

// `SCardGetStatusChange()` call from JS fails when using a wrong context.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardGetStatusChangeWrongContext) {
  constexpr SCARDCONTEXT kWrongScardContext = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  std::vector<Value> reader_states;
  LONG return_code = SimulateGetStatusChangeCallFromJsClient(
      kFakeHandlerId, kWrongScardContext,
      /*timeout=*/INFINITE,
      ArrayValueBuilder()
          .Add(DictValueBuilder()
                   .Add("reader_name", kGemaltoPcTwinReaderPcscName0)
                   .Add("current_state", SCARD_STATE_EMPTY)
                   .Get())
          .Get(),
      reader_states);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
  EXPECT_THAT(reader_states, IsEmpty());
}

// `SCardCancel()` call from JS terminates a running `SCardGetStatusChange()`
// call.
TEST_F(SmartCardConnectorApplicationSingleClientTest, Cancel) {
  // Arrange:
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  // Start a blocking `SCardGetStatusChange()` call on a different thread.
  std::future<LONG> pending_status_return_code =
      std::async(std::launch::async, [&] {
        std::vector<Value> reader_states;
        return SimulateGetStatusChangeCallFromJsClient(
            kFakeHandlerId, scard_context(),
            /*timeout=*/INFINITE,
            ArrayValueBuilder()
                .Add(DictValueBuilder()
                         .Add("reader_name", kPnpNotification)
                         .Add("current_state", SCARD_STATE_UNAWARE)
                         .Get())
                .Get(),
            reader_states);
      });
  // Check that the call is actually blocked (either until a reader event or
  // cancellation happen). The exact interval isn't important here - we just
  // want some reasonably big probability of catching a bug if it's introduced.
  EXPECT_EQ(pending_status_return_code.wait_for(std::chrono::seconds(1)),
            std::future_status::timeout);

  // Act: trigger `SCardCancel()` to abort the blocking call.
  LONG cancellation_return_code =
      SimulateCancelCallFromJsClient(kFakeHandlerId, scard_context());
  // Wait until the `SCardGetStatusChange()` call completes.
  LONG status_return_code = pending_status_return_code.get();

  // Assert:
  EXPECT_EQ(cancellation_return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(status_return_code, SCARD_E_CANCELLED);
}

// `SCardCancel()` call from JS succeeds even when there's no pending
// `SCardGetStatusChange()` call.
TEST_F(SmartCardConnectorApplicationSingleClientTest, CancelNothing) {
  // Arrange:
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act:
  LONG return_code =
      SimulateCancelCallFromJsClient(kFakeHandlerId, scard_context());

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
}

// `SCardCancel()` call from JS fails when using a wrong context.
TEST_F(SmartCardConnectorApplicationSingleClientTest, CancelWrongContext) {
  constexpr SCARDCONTEXT kWrongScardContext = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  LONG return_code =
      SimulateCancelCallFromJsClient(kFakeHandlerId, kWrongScardContext);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
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

// `SCardConnect()` call from JS fails when using a wrong context.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardConnectWrongContext) {
  constexpr SCARDCONTEXT kWrongScardContext = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  LONG return_code = SimulateConnectCallFromJsClient(
      kFakeHandlerId, kWrongScardContext, kGemaltoPcTwinReaderPcscName0,
      SCARD_SHARE_SHARED,
      /*preferred_protocols=*/SCARD_PROTOCOL_RAW, scard_handle,
      active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
  EXPECT_EQ(scard_handle, 0);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(0));
}

namespace {

struct ReaderTestParam {
  TestingSmartCardSimulation::DeviceType device_type;
  TestingSmartCardSimulation::CardType card_type;
  const char* reader_pcsc_name;

  ReaderTestParam(TestingSmartCardSimulation::DeviceType device_type,
                  TestingSmartCardSimulation::CardType card_type,
                  const char* reader_pcsc_name)
      : device_type(device_type),
        card_type(card_type),
        reader_pcsc_name(reader_pcsc_name) {}
};

}  // namespace

// Parameterized test fixture that's instantiated for every device that we can
// emulate. Useful for testing that basic scenarios work across different types
// of readers (we don't parameterize other tests in this file as this'd make a
// single test run very long).
class SmartCardConnectorApplicationReaderCompatibilityTest
    : public SmartCardConnectorApplicationSingleClientTest,
      public ::testing::WithParamInterface<ReaderTestParam> {};

// Subset of the above, for readers that don't have the card built-in.
class SmartCardConnectorApplicationReaderWithoutBuiltinCardCompatibilityTest
    : public SmartCardConnectorApplicationSingleClientTest,
      public ::testing::WithParamInterface<ReaderTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    AllDevices,
    SmartCardConnectorApplicationReaderCompatibilityTest,
    ::testing::Values(
        ReaderTestParam(
            TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader,
            TestingSmartCardSimulation::CardType::kCosmoId70,
            kGemaltoPcTwinReaderPcscName0),
        ReaderTestParam(TestingSmartCardSimulation::DeviceType::
                            kDellSmartCardReaderKeyboard,
                        TestingSmartCardSimulation::CardType::kCosmoId70,
                        kDellSmartCardReaderKeyboardPcscName0),
        ReaderTestParam(TestingSmartCardSimulation::DeviceType::kYubikey4C,
                        TestingSmartCardSimulation::CardType::kYubikey,
                        kYubikey4CPcscName0)));

INSTANTIATE_TEST_SUITE_P(
    AllDevices,
    SmartCardConnectorApplicationReaderWithoutBuiltinCardCompatibilityTest,
    ::testing::Values(
        ReaderTestParam(
            TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader,
            TestingSmartCardSimulation::CardType::kCosmoId70,
            kGemaltoPcTwinReaderPcscName0),
        ReaderTestParam(TestingSmartCardSimulation::DeviceType::
                            kDellSmartCardReaderKeyboard,
                        TestingSmartCardSimulation::CardType::kCosmoId70,
                        kDellSmartCardReaderKeyboardPcscName0)));

// Test that the reader and card are successfully initialized, and that
// corresponding events are sent when they're inserted/removed.
TEST_P(SmartCardConnectorApplicationReaderWithoutBuiltinCardCompatibilityTest,
       Basic) {
  // Start up with no readers.
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Plug in the reader.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = GetParam().device_type;
  SetUsbDevices({device});
  // Wait until SCardGetStatusChange reports the change in the list of readers.
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
  // Check that the reader is present in the list now.
  std::vector<std::string> readers;
  EXPECT_EQ(SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                                /*groups=*/Value(), readers),
            SCARD_S_SUCCESS);
  EXPECT_THAT(readers, ElementsAre(GetParam().reader_pcsc_name));

  // Insert the smart card.
  device.card_type = GetParam().card_type;
  SetUsbDevices({device});
  // Wait until SCardGetStatusChange reports the change of the reader state.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", GetParam().reader_pcsc_name)
                             .Add("current_state", SCARD_STATE_EMPTY)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  // Check that the card presence has been reported. Depending on timing
  // (specifically, whether the reader monitoring thread starts before/after the
  // card insertion), PC/SC-Lite may or may not return the additional counter in
  // the higher 16 bits of the "event_state" field.
  ASSERT_THAT(reader_states, SizeIs(1));
  EXPECT_THAT(
      reader_states[0],
      AnyOf(DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_PRESENT | 0x10000),
            DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_PRESENT)));
  EXPECT_THAT(reader_states[0],
              DictContains("atr", TestingSmartCardSimulation::GetCardAtr(
                                      GetParam().card_type)));

  // Remove the card.
  device.card_type.reset();
  SetUsbDevices({device});
  // Wait until SCardGetStatusChange reports the change of the reader state.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", GetParam().reader_pcsc_name)
                             .Add("current_state", SCARD_STATE_PRESENT)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  // Check that the card absence has been reported.
  ASSERT_THAT(reader_states, SizeIs(1));
  // The "event_state" field contains the number of card insertion/removal
  // events in the higher 16 bits. See the comment above on why its value can
  // diverge.
  EXPECT_THAT(
      reader_states[0],
      AnyOf(DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_EMPTY | 0x20000),
            DictContains("event_state",
                         SCARD_STATE_CHANGED | SCARD_STATE_EMPTY | 0x10000)));
  EXPECT_THAT(reader_states[0], DictContains("atr", Value::Type::kBinary));

  // Unplug the reader.
  SetUsbDevices({});
  // Wait until SCardGetStatusChange reports the change in the list of readers.
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
  // Check that the reader list is empty now.
  EXPECT_EQ(SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                                /*groups=*/Value(), readers),
            SCARD_E_NO_READERS_AVAILABLE);
}

// Test that repeated card insertions and removals are handled correctly.
TEST_P(SmartCardConnectorApplicationReaderWithoutBuiltinCardCompatibilityTest,
       RepeatedInsertRemove) {
  constexpr int kIterationCount = 10;  // chosen semi-arbitrarily

  // Arrange: start with an empty reader.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = GetParam().device_type;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act/assert: iterations of insertion/removal with connect/disconnect calls.
  for (int iter = 0; iter < kIterationCount; ++iter) {
    // Insert the card and wait until it's recognized.
    device.card_type = GetParam().card_type;
    SetUsbDevices({device});
    std::vector<Value> reader_states;
    EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                  kFakeHandlerId, scard_context(),
                  /*timeout=*/INFINITE,
                  ArrayValueBuilder()
                      .Add(DictValueBuilder()
                               .Add("reader_name", GetParam().reader_pcsc_name)
                               .Add("current_state", SCARD_STATE_EMPTY)
                               .Get())
                      .Get(),
                  reader_states),
              SCARD_S_SUCCESS);
    ASSERT_THAT(reader_states, SizeIs(1));
    EXPECT_THAT(
        reader_states[0],
        DictContainsLike("event_state",
                         IsIntegerValue(HasBitsSet(SCARD_STATE_PRESENT))));

    // Connect to the card.
    SCARDHANDLE scard_handle = 0;
    DWORD active_protocol = 0;
    EXPECT_EQ(SimulateConnectCallFromJsClient(
                  kFakeHandlerId, scard_context(), GetParam().reader_pcsc_name,
                  SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, scard_handle,
                  active_protocol),
              SCARD_S_SUCCESS);
    EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));

    // Disconnect from the card.
    EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                                 SCARD_LEAVE_CARD),
              SCARD_S_SUCCESS);

    // Remove the card.
    device.card_type.reset();
    SetUsbDevices({device});
    EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                  kFakeHandlerId, scard_context(),
                  /*timeout=*/INFINITE,
                  ArrayValueBuilder()
                      .Add(DictValueBuilder()
                               .Add("reader_name", GetParam().reader_pcsc_name)
                               .Add("current_state", SCARD_STATE_PRESENT)
                               .Get())
                      .Get(),
                  reader_states),
              SCARD_S_SUCCESS);
    ASSERT_THAT(reader_states, SizeIs(1));
    EXPECT_THAT(
        reader_states[0],
        DictContainsLike("event_state",
                         IsIntegerValue(HasBitsSet(SCARD_STATE_EMPTY))));
  }
}

// Test that the reader (with an always-inserted card) is successfully
// initialized, and that corresponding events are sent when it's
// attached/removed.
TEST_P(SmartCardConnectorApplicationReaderCompatibilityTest, Basic) {
  // Start up with no readers.
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Plug in the reader (with the card inserted, as it's built into the reader).
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = GetParam().device_type;
  device.card_type = GetParam().card_type;
  SetUsbDevices({device});
  // Wait until SCardGetStatusChange reports the change in the list of readers.
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
  // Check that the reader is present in the list now.
  std::vector<std::string> readers;
  EXPECT_EQ(SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                                /*groups=*/Value(), readers),
            SCARD_S_SUCCESS);
  EXPECT_THAT(readers, ElementsAre(GetParam().reader_pcsc_name));
  // Check that the SCardGetStatusChange for that reader immediately reports the
  // card as being present.
  EXPECT_EQ(SimulateGetStatusChangeCallFromJsClient(
                kFakeHandlerId, scard_context(),
                /*timeout=*/INFINITE,
                ArrayValueBuilder()
                    .Add(DictValueBuilder()
                             .Add("reader_name", GetParam().reader_pcsc_name)
                             .Add("current_state", SCARD_STATE_UNAWARE)
                             .Get())
                    .Get(),
                reader_states),
            SCARD_S_SUCCESS);
  ASSERT_THAT(reader_states, SizeIs(1));
  EXPECT_THAT(
      reader_states[0],
      DictContains("event_state", SCARD_STATE_CHANGED | SCARD_STATE_PRESENT));
  EXPECT_THAT(reader_states[0],
              DictContains("atr", TestingSmartCardSimulation::GetCardAtr(
                                      GetParam().card_type)));

  // Test connecting to the card.
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), GetParam().reader_pcsc_name,
                SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));
  // Disconnect from the card.
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);

  // Unplug the reader.
  SetUsbDevices({});
  // Wait until SCardGetStatusChange reports the change in the list of readers.
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
  // Check that the reader list is empty now.
  EXPECT_EQ(SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                                /*groups=*/Value(), readers),
            SCARD_E_NO_READERS_AVAILABLE);
}

// Test fixture that sets up a test reader with a card inserted into it, and a
// client that has open `SCARDCONTEXT` and `SCARDHANDLE` for the reader.
class SmartCardConnectorApplicationConnectedReaderTest
    : public SmartCardConnectorApplicationSingleClientTest {
 protected:
  void SetUp() override {
    SmartCardConnectorApplicationSingleClientTest::SetUp();

    SetUsbDevices({GetSimulationDevice()});
    StartApplication();
    SetUpJsClient();
    SetUpSCardContext();

    DWORD active_protocol = 0;
    EXPECT_EQ(SimulateConnectCallFromJsClient(
                  kFakeHandlerId, scard_context(),
                  kGemaltoPcTwinReaderPcscName0, SCARD_SHARE_SHARED,
                  /*preferred_protocols=*/SCARD_PROTOCOL_T1, scard_handle_,
                  active_protocol),
              SCARD_S_SUCCESS);
    EXPECT_EQ(active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));
  }

  void TearDown() override {
    EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle_,
                                                 SCARD_LEAVE_CARD),
              SCARD_S_SUCCESS);
    SmartCardConnectorApplicationSingleClientTest::TearDown();
  }

  TestingSmartCardSimulation::Device GetSimulationDevice() const {
    TestingSmartCardSimulation::Device device;
    device.id = 123;
    device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
    device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
    return device;
  }

  SCARDHANDLE scard_handle() const { return scard_handle_; }

 private:
  SCARDHANDLE scard_handle_;
};

// `SCardReconnect()` call from JS succeeds when using the same parameters as
// the previous `SCardConnect()` call.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, SCardReconnect) {
  // Reconnect using the same sharing and protocol as the `SCardConnect()` call
  // in the test's setup.
  DWORD new_active_protocol = 0;
  LONG return_code = SimulateReconnectCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_SHARE_SHARED,
      /*preferred_protocols=*/SCARD_PROTOCOL_ANY, SCARD_LEAVE_CARD,
      new_active_protocol);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(new_active_protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));
}

// `SCardReconnect()` call from JS fails when using a wrong handle.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       SCardReconnectWrongHandle) {
  constexpr SCARDHANDLE kWrongScardHandle = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  DWORD active_protocol = 0;
  LONG return_code = SimulateReconnectCallFromJsClient(
      kFakeHandlerId, kWrongScardHandle, SCARD_SHARE_SHARED,
      /*preferred_protocols=*/SCARD_PROTOCOL_ANY, SCARD_LEAVE_CARD,
      active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
  EXPECT_EQ(active_protocol, static_cast<DWORD>(0));
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

// `SCardDisconnect()` calls from JS should fail when using a wrong handle.
TEST_F(SmartCardConnectorApplicationSingleClientTest, DisconnectWrongHandle) {
  constexpr SCARDHANDLE kWrongScardHandle = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  LONG return_code = SimulateDisconnectCallFromJsClient(
      kFakeHandlerId, kWrongScardHandle, SCARD_LEAVE_CARD);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardStatus()` calls from JS should succeed and return information about the
// card.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, Status) {
  // Act:
  std::string reader_name;
  DWORD state = 0;
  DWORD protocol = 0;
  std::vector<uint8_t> atr;
  LONG return_code = SimulateStatusCallFromJsClient(
      kFakeHandlerId, scard_handle(), reader_name, state, protocol, atr);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(reader_name, kGemaltoPcTwinReaderPcscName0);
  EXPECT_EQ(state, static_cast<DWORD>(SCARD_NEGOTIABLE | SCARD_POWERED |
                                      SCARD_PRESENT));
  EXPECT_EQ(protocol, static_cast<DWORD>(SCARD_PROTOCOL_T1));
  EXPECT_EQ(atr, TestingSmartCardSimulation::GetCardAtr(
                     TestingSmartCardSimulation::CardType::kCosmoId70));
}

// `SCardStatus()` starts returning `SCARD_E_READER_UNAVAILABLE` after the
// reader disappears.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       StatusAfterReaderRemoval) {
  // Act:
  SetUsbDevices({});

  // Assert: `SCardStatus()` should eventually return a specific error (we have
  // to do a polling loop since there's no simple way to observe this
  // asynchronous event).
  WaitUntilPredicate([&]() {
    std::string reader_name;
    DWORD state = 0;
    DWORD protocol = 0;
    std::vector<uint8_t> atr;
    LONG return_code = SimulateStatusCallFromJsClient(
        kFakeHandlerId, scard_handle(), reader_name, state, protocol, atr);
    // Continue waiting if `SCARD_S_SUCCESS` was returned. Complete the test
    // after `SCARD_E_READER_UNAVAILABLE` is returned, but verify the returned
    // values are correct.
    EXPECT_THAT(return_code,
                AnyOf(SCARD_S_SUCCESS, SCARD_E_READER_UNAVAILABLE));
    if (return_code == SCARD_S_SUCCESS)
      return false;
    EXPECT_EQ(reader_name, "");
    EXPECT_EQ(state, static_cast<DWORD>(0));
    EXPECT_EQ(protocol, static_cast<DWORD>(0));
    EXPECT_THAT(atr, IsEmpty());
    return true;
  });
}

// `SCardStatus()` starts returning `SCARD_W_REMOVED_CARD` after the card gets
// removed from the reader.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       StatusAfterCardRemoval) {
  // Act: simulate the card removal.
  TestingSmartCardSimulation::Device device = GetSimulationDevice();
  device.card_type = {};
  SetUsbDevices({device});

  // Assert: `SCardStatus()` should eventually return a specific error (we have
  // to do a polling loop since there's no simple way to observe this
  // asynchronous event).
  WaitUntilPredicate([&]() {
    std::string reader_name;
    DWORD state = 0;
    DWORD protocol = 0;
    std::vector<uint8_t> atr;
    LONG return_code = SimulateStatusCallFromJsClient(
        kFakeHandlerId, scard_handle(), reader_name, state, protocol, atr);
    // Continue waiting if `SCARD_S_SUCCESS` was returned. Complete the test
    // after `SCARD_W_REMOVED_CARD` is returned, but verify the returned values
    // are correct.
    EXPECT_THAT(return_code, AnyOf(SCARD_S_SUCCESS, SCARD_W_REMOVED_CARD));
    if (return_code == SCARD_S_SUCCESS)
      return false;
    EXPECT_EQ(reader_name, "");
    EXPECT_EQ(state, static_cast<DWORD>(0));
    EXPECT_EQ(protocol, static_cast<DWORD>(0));
    EXPECT_THAT(atr, IsEmpty());
    return true;
  });
}

// `SCardStatus()` calls from JS should fail when using a wrong handle.
TEST_F(SmartCardConnectorApplicationSingleClientTest, StatusWrongHandle) {
  constexpr SCARDHANDLE kWrongScardHandle = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  std::string reader_name;
  DWORD state = 0;
  DWORD protocol = 0;
  std::vector<uint8_t> atr;
  LONG return_code = SimulateStatusCallFromJsClient(
      kFakeHandlerId, kWrongScardHandle, reader_name, state, protocol, atr);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardGetAttrib()` call from JS should succeed for the
// `SCARD_ATTR_ATR_STRING` argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, GetAttribAtr) {
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_ATR_STRING, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(attr, TestingSmartCardSimulation::GetCardAtr(
                      TestingSmartCardSimulation::CardType::kCosmoId70));
}

// `SCardGetAttrib()` call from JS should return the single byte "1" for the
// `SCARD_ATTR_ICC_INTERFACE_STATUS` argument when the card is present.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       GetAttribInterfaceStatusPresent) {
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_ICC_INTERFACE_STATUS, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(attr, ElementsAre(1));
}

// `SCardGetAttrib()` call from JS should return the single byte "0" for the
// `SCARD_ATTR_ICC_INTERFACE_STATUS` argument when the card is missing.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       GetAttribInterfaceStatusMissing) {
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_DIRECT,
                /*preferred_protocols=*/0, scard_handle, active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_EQ(active_protocol, 0U);

  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle, SCARD_ATTR_ICC_INTERFACE_STATUS, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(attr, ElementsAre(0));
}

// `SCardGetAttrib()` call from JS should return the single byte "2" for the
// `SCARD_ATTR_ICC_PRESENCE` argument when the card is present.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, GetAttribIccPresence) {
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_ICC_PRESENCE, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(attr, ElementsAre(2));
}

// `SCardGetAttrib()` call from JS should return the single byte "0" for the
// `SCARD_ATTR_ICC_PRESENCE` argument when the card is missing.
TEST_F(SmartCardConnectorApplicationSingleClientTest, GetAttribIccPresence) {
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_DIRECT,
                /*preferred_protocols=*/0, scard_handle, active_protocol),
            SCARD_S_SUCCESS);
  EXPECT_EQ(active_protocol, 0U);

  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle, SCARD_ATTR_ICC_PRESENCE, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(attr, ElementsAre(0));
}

// `SCardGetAttrib()` call from JS should return the four-byte device version
// for the `SCARD_ATTR_VENDOR_IFD_VERSION` argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       GetAttribVendorIfdVersion) {
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_VENDOR_IFD_VERSION, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  // The value corresponds to the `version` field set for `kGemaltoPcTwinReader`
  // in testing_smart_card_simulation.cc (padded with all-zero build numbers and
  // in the opposite endianness).
  EXPECT_THAT(attr, ElementsAre(0x00, 0x00, 0x00, 0x02));
}

// `SCardGetAttrib()` call from JS should return an empty string for the
// `SCARD_ATTR_VENDOR_NAME` argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, GetAttribVendorName) {
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_VENDOR_NAME, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(attr, IsEmpty());
}

// `SCardGetAttrib()` call from JS should return the expected number for the
// `SCARD_ATTR_MAXINPUT` argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, GetAttribMaxinput) {
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_MAXINPUT, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  // This value corresponds to the data encoded in the `kGemaltoPcTwinReader`'s
  // device descriptor in testing_smart_card_simulation.cc.
  EXPECT_THAT(attr, ElementsAre(0x05, 0x01, 0x00, 0x00));
}

// `SCardGetAttrib()` call from JS should return an empty string for the
// `SCARD_ATTR_VENDOR_IFD_SERIAL_NO` argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       GetAttribVendorIfdSerialNo) {
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_VENDOR_IFD_SERIAL_NO, attr);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_THAT(attr, IsEmpty());
}

// `SCardGetAttrib()` call from JS should fail when using a wrong handle.
TEST_F(SmartCardConnectorApplicationSingleClientTest, GetAttribWrongHandle) {
  constexpr SCARDHANDLE kWrongScardHandle = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  std::vector<uint8_t> attr;
  LONG return_code = SimulateGetAttribCallFromJsClient(
      kFakeHandlerId, kWrongScardHandle, SCARD_ATTR_ATR_STRING, attr);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardSetAttrib()` calls from JS should succeed for `TAG_IFD_DEVICE_REMOVED`
// and a single-byte zero blob argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       SetAttribSuccessIfdDeviceRemovedWithSingleZero) {
  // TAG_IFD_DEVICE_REMOVED with this parameter is a no-op, however the result
  // is still "success".
  LONG return_code = SimulateSetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), kTagIfdDeviceRemoved,
      std::vector<uint8_t>{0});
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
}

// `SCardSetAttrib()` calls from JS should succeed for `TAG_IFD_DEVICE_REMOVED`
// and a multiple-byte blob argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       SetAttribSuccessIfdDeviceRemovedWithMultipleBytes) {
  // TAG_IFD_DEVICE_REMOVED with this parameter is a no-op, however the result
  // is still "success".
  LONG return_code = SimulateSetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), kTagIfdDeviceRemoved,
      std::vector<uint8_t>{0, 1, 2});
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
}

// `SCardSetAttrib()` calls from JS should fail for a supported attribute
// (`TAG_IFD_DEVICE_REMOVED`) and an empty-blob argument.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       SetAttribErrorIfdDeviceRemovedWithEmptyParam) {
  LONG return_code = SimulateSetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), kTagIfdDeviceRemoved,
      std::vector<uint8_t>{});
  EXPECT_EQ(return_code, SCARD_E_INVALID_PARAMETER);
}

// `SCardSetAttrib()` calls from JS should fail for a supported attribute
// (`TAG_IFD_DEVICE_REMOVED`) and an argument that exceeds the maximum allowed
// size.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       SetAttribErrorIfdDeviceRemovedWithOversizedParam) {
  // The exact value doesn't matter, but it should exceed the `MAX_BUFFER_SIZE`
  // constant in the PC/SC implementation (which is 264 at the moment).
  const size_t kHugeSize = 10 * 1000;

  LONG return_code = SimulateSetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), kTagIfdDeviceRemoved,
      std::vector<uint8_t>(kHugeSize));
  EXPECT_EQ(return_code, SCARD_E_INSUFFICIENT_BUFFER);
}

// `SCardSetAttrib()` calls from JS should fail for unsupported attributes.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       SetAttribErrorUnsupportedAttribute) {
  LONG return_code = SimulateSetAttribCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_ATTR_ATR_STRING,
      /*data_to_send=*/{});
  EXPECT_EQ(return_code, SCARD_E_INVALID_PARAMETER);
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

// `SCardTransmit()` calls from JS should fail when using a wrong handle.
TEST_F(SmartCardConnectorApplicationSingleClientTest, TransmitWrongHandle) {
  constexpr SCARDHANDLE kWrongScardHandle = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  std::vector<uint8_t> response;
  DWORD response_protocol = 0;
  LONG return_code = SimulateTransmitCallFromJsClient(
      kFakeHandlerId, kWrongScardHandle, SCARD_PROTOCOL_T1,
      /*data_to_send=*/{1, 2, 3, 4},
      /*receive_protocol=*/{}, response_protocol, response);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardBeginTransaction()` calls from JS should succeed.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, BeginTransaction) {
  LONG return_code =
      SimulateBeginTransactionCallFromJsClient(kFakeHandlerId, scard_handle());

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);

  // The test `TearDown()` will verify that the disconnection works despite the
  // unended transaction.
}

// `SCardBeginTransaction()` calls from JS should fail when using a wrong
// handle.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       BeginTransactionWrongHandle) {
  constexpr SCARDHANDLE kWrongScardHandle = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  LONG return_code = SimulateBeginTransactionCallFromJsClient(
      kFakeHandlerId, kWrongScardHandle);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardEndTransaction()` calls from JS should succeed.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, EndTransaction) {
  // Arrange: begin a transaction.
  EXPECT_EQ(
      SimulateBeginTransactionCallFromJsClient(kFakeHandlerId, scard_handle()),
      SCARD_S_SUCCESS);

  // Act:
  LONG return_code = SimulateEndTransactionCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_LEAVE_CARD);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
}

// `SCardEndTransaction()` calls from JS should fail if there's no previous
// `SCardBeginTransaction()` call.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       EndTransactionWithoutBegin) {
  // Act:
  LONG return_code = SimulateEndTransactionCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_LEAVE_CARD);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_NOT_TRANSACTED);
}

// `SCardEndTransaction()` calls from JS should fail if it's called more than
// once for a single transaction.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       EndTransactionDuplicate) {
  // Arrange: begin and end a transaction.
  EXPECT_EQ(
      SimulateBeginTransactionCallFromJsClient(kFakeHandlerId, scard_handle()),
      SCARD_S_SUCCESS);
  EXPECT_EQ(SimulateEndTransactionCallFromJsClient(
                kFakeHandlerId, scard_handle(), SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);

  // Act:
  LONG return_code = SimulateEndTransactionCallFromJsClient(
      kFakeHandlerId, scard_handle(), SCARD_LEAVE_CARD);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_NOT_TRANSACTED);
}

// `SCardEndTransaction()` calls from JS should fail when using a wrong handle.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       EndTransactionWrongHandle) {
  constexpr SCARDHANDLE kWrongScardHandle = 123456;

  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  LONG return_code = SimulateEndTransactionCallFromJsClient(
      kFakeHandlerId, kWrongScardHandle, SCARD_LEAVE_CARD);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_INVALID_HANDLE);
}

// `SCardControl()` call from JS should succeed for the
// `CM_IOCTL_GET_FEATURE_REQUEST` command and return the list of features
// supported by the reader.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, ControlGetFeature) {
  // A TLV ("tag-length-value") structure that contains the PCSC_TLV_STRUCTURE
  // constant. For the test reader it's the only expected blob to be returned.
  const std::vector<uint8_t> kFeatureGetTlvProperties = {0x12, 0x04, 0x42,
                                                         0x33, 0x00, 0x12};

  std::vector<uint8_t> received_data;
  LONG return_code = SimulateControlCallFromJsClient(
      kFakeHandlerId, scard_handle(), CM_IOCTL_GET_FEATURE_REQUEST,
      /*data_to_send=*/{}, received_data);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(received_data, kFeatureGetTlvProperties);
}

// `SCardControl()` call from JS should fail for the
// `IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE` command.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       ControlVendorIfdFailure) {
  // Corresponds to `IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE` in the CCID sources.
  constexpr DWORD kIoctlSmartcardVendorIfdExchange = 0x42000001;

  std::vector<uint8_t> received_data;
  LONG return_code = SimulateControlCallFromJsClient(
      kFakeHandlerId, scard_handle(), kIoctlSmartcardVendorIfdExchange,
      /*data_to_send=*/{}, received_data);

  EXPECT_EQ(return_code, SCARD_E_NOT_TRANSACTED);
  EXPECT_THAT(received_data, IsEmpty());
}

// `SCardControl()` call from JS should succeed for the
// `IOCTL_FEATURE_IFD_PIN_PROPERTIES` command and return the properties.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest, ControlFeatureIfdPin) {
  // Corresponds to `IOCTL_FEATURE_IFD_PIN_PROPERTIES` in the CCID sources.
  constexpr DWORD kIoctlFeatureIfdPinProperties = 0x4233000A;
  // The `PIN_PROPERTIES_STRUCTURE` struct as encoded blob, with the value
  // that's expected for a standard reader.
  const std::vector<uint8_t> kPinPropertiesStructure = {0x00, 0x00, 0x07, 0x00};

  std::vector<uint8_t> received_data;
  LONG return_code = SimulateControlCallFromJsClient(
      kFakeHandlerId, scard_handle(), kIoctlFeatureIfdPinProperties,
      /*data_to_send=*/{}, received_data);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(received_data, kPinPropertiesStructure);
}

// `SCardControl()` call from JS should succeed for the
// `IOCTL_FEATURE_GET_TLV_PROPERTIES` command and return the properties.
TEST_F(SmartCardConnectorApplicationConnectedReaderTest,
       ControlFeatureGetTlvProperties) {
  // Corresponds to `IOCTL_FEATURE_GET_TLV_PROPERTIES` in the CCID sources.
  constexpr DWORD kIoctlFeatureGetTlvProperties = 0x42330012;
  const std::vector<uint8_t> kTlvProperties = {
      0x01, 0x02, 0x00, 0x00, 0x03, 0x01, 0x00, 0x09, 0x01, 0x00, 0x0B, 0x02,
      0xE6, 0x08, 0x0C, 0x02, 0x37, 0x34, 0x0A, 0x04, 0x00, 0x00, 0x01, 0x00};

  std::vector<uint8_t> received_data;
  LONG return_code = SimulateControlCallFromJsClient(
      kFakeHandlerId, scard_handle(), kIoctlFeatureGetTlvProperties,
      /*data_to_send=*/{}, received_data);

  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_EQ(received_data, kTlvProperties);
}

class SmartCardConnectorApplicationTwoClientsTest
    : public SmartCardConnectorApplicationSingleClientTest {
 protected:
  static constexpr int kFakeSecondHandlerId = 4567;
  static constexpr const char* kFakeSecondClientNameForLog =
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

  void TearDown() override {
    if (second_scard_context_)
      TearDownSecondSCardContext();
    if (second_js_client_setup_)
      SimulateJsClientRemoved(kFakeSecondHandlerId);

    SmartCardConnectorApplicationSingleClientTest::TearDown();
  }

  void SetUpSecondJsClient() {
    GOOGLE_SMART_CARD_CHECK(!second_js_client_setup_);
    SimulateJsClientAdded(kFakeSecondHandlerId, kFakeSecondClientNameForLog);
    second_js_client_setup_ = true;
  }

  void SetUpSecondSCardContext() {
    GOOGLE_SMART_CARD_CHECK(!second_scard_context_);
    SCARDCONTEXT local_scard_context = 0;
    EXPECT_EQ(SimulateEstablishContextCallFromJsClient(
                  kFakeSecondHandlerId, SCARD_SCOPE_SYSTEM,
                  /*reserved1=*/Value(),
                  /*reserved2=*/Value(), local_scard_context),
              SCARD_S_SUCCESS);
    second_scard_context_ = local_scard_context;
  }

  void TearDownSecondSCardContext() {
    GOOGLE_SMART_CARD_CHECK(second_scard_context_);
    EXPECT_EQ(SimulateReleaseContextCallFromJsClient(kFakeSecondHandlerId,
                                                     *second_scard_context_),
              SCARD_S_SUCCESS);
    second_scard_context_ = {};
  }

  SCARDCONTEXT second_scard_context() const { return *second_scard_context_; }

 private:
  bool second_js_client_setup_ = false;
  optional<SCARDCONTEXT> second_scard_context_;
};

// `SCardConnect()` call from JS succeeds even when there's an active connection
// from another client (which allows shared access).
TEST_F(SmartCardConnectorApplicationTwoClientsTest, ConnectConcurrent) {
  // Arrange: set up a reader and a card.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  // Set up the first client, which holds an shared connection.
  SetUpJsClient();
  SetUpSCardContext();
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_SHARED,
                /*preferred_protocols=*/SCARD_PROTOCOL_ANY, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  // Set up the second client.
  SetUpSecondJsClient();
  SetUpSecondSCardContext();

  // Act: the second client attempts to connect to the card.
  SCARDHANDLE second_scard_handle = 0;
  LONG return_code = SimulateConnectCallFromJsClient(
      kFakeSecondHandlerId, second_scard_context(),
      kGemaltoPcTwinReaderPcscName0, SCARD_SHARE_SHARED,
      /*preferred_protocols=*/SCARD_PROTOCOL_ANY, second_scard_handle,
      active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_S_SUCCESS);
  EXPECT_NE(scard_handle, second_scard_handle);

  // Cleanup.
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(
                kFakeSecondHandlerId, second_scard_handle, SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// `SCardConnect()` call from JS fails if there's another client holding
// exclusive access.
TEST_F(SmartCardConnectorApplicationTwoClientsTest,
       ConnectErrorOtherExclusive) {
  // Arrange: set up a reader and a card.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.card_type = TestingSmartCardSimulation::CardType::kCosmoId70;
  SetUsbDevices({device});
  StartApplication();
  // Set up the first client, which holds an exclusive connection.
  SetUpJsClient();
  SetUpSCardContext();
  SCARDHANDLE scard_handle = 0;
  DWORD active_protocol = 0;
  EXPECT_EQ(SimulateConnectCallFromJsClient(
                kFakeHandlerId, scard_context(), kGemaltoPcTwinReaderPcscName0,
                SCARD_SHARE_EXCLUSIVE,
                /*preferred_protocols=*/SCARD_PROTOCOL_ANY, scard_handle,
                active_protocol),
            SCARD_S_SUCCESS);
  // Set up the second client.
  SetUpSecondJsClient();
  SetUpSecondSCardContext();

  // Act: the second client attempts to connect to the card.
  SCARDHANDLE second_scard_handle = 0;
  LONG return_code = SimulateConnectCallFromJsClient(
      kFakeSecondHandlerId, second_scard_context(),
      kGemaltoPcTwinReaderPcscName0, SCARD_SHARE_SHARED,
      /*preferred_protocols=*/SCARD_PROTOCOL_ANY, second_scard_handle,
      active_protocol);

  // Assert:
  EXPECT_EQ(return_code, SCARD_E_SHARING_VIOLATION);
  EXPECT_EQ(second_scard_handle, 0);

  // Cleanup.
  EXPECT_EQ(SimulateDisconnectCallFromJsClient(kFakeHandlerId, scard_handle,
                                               SCARD_LEAVE_CARD),
            SCARD_S_SUCCESS);
}

// Test that the PC/SC server shuts down timely in the scenario when a new
// reader is added shortly before that.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       ReaderAddedBeforeShutdown) {
  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});

  // Nothing to assert explicitly here - the relevant assertion is in the test
  // fixture destructor that verifies that the shutdown fits into a timeout.
}

// Same as above, but additionally makes a PC/SC call roundtrip to affect the
// timings and increase the probabilty of catching bugs if they appear.
TEST_F(SmartCardConnectorApplicationSingleClientTest,
       ReaderAddedBeforeShutdownWithPcscCall) {
  // Arrange:
  StartApplication();
  SetUpJsClient();

  // Act:
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  SetUsbDevices({device});
  SetUpSCardContext();

  // Nothing to assert explicitly here - see the comment in the previous test.
}

class SmartCardConnectorApplicationReaderErrorTest
    : public SmartCardConnectorApplicationSingleClientTest,
      public ::testing::WithParamInterface<
          TestingSmartCardSimulation::ErrorMode> {};

INSTANTIATE_TEST_SUITE_P(
    AllErrorModes,
    SmartCardConnectorApplicationReaderErrorTest,
    ::testing::Values(
        TestingSmartCardSimulation::ErrorMode::kOpenDeviceError,
        TestingSmartCardSimulation::ErrorMode::kClaimInterfaceError,
        TestingSmartCardSimulation::ErrorMode::kBulkTransferError,
        TestingSmartCardSimulation::ErrorMode::kBulkTransferUnrelatedReplies));

// Reader initialization fails when there's a persistent USB error.
TEST_P(SmartCardConnectorApplicationReaderErrorTest, ReaderInitFailure) {
  // Arrange:
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act: simulate a reader with a persistent error.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.error_mode = GetParam();
  SetUsbDevices({device});

  // Assert:
  EXPECT_EQ(reader_notification_observer().WaitAndPop(),
            "reader_init_add:Gemalto PC Twin Reader");
  EXPECT_EQ(reader_notification_observer().WaitAndPop(),
            "reader_finish_add:Gemalto PC Twin Reader:error");
  std::vector<std::string> readers;
  EXPECT_EQ(SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                                /*groups=*/Value(), readers),
            SCARD_E_NO_READERS_AVAILABLE);
}

namespace {

struct TransientErrorTestParam {
  TransientErrorTestParam(
      TestingSmartCardSimulation::ErrorMode error_mode,
      TestingSmartCardSimulation::ErrorCessation error_cessation)
      : error_mode(error_mode), error_cessation(error_cessation) {}

  TestingSmartCardSimulation::ErrorMode error_mode;
  TestingSmartCardSimulation::ErrorCessation error_cessation;
};

}  // namespace

class SmartCardConnectorApplicationReaderTransientErrorTest
    : public SmartCardConnectorApplicationSingleClientTest,
      public ::testing::WithParamInterface<TransientErrorTestParam> {};

INSTANTIATE_TEST_SUITE_P(
    AllErrorModes,
    SmartCardConnectorApplicationReaderTransientErrorTest,
    ::testing::Values(
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::kOpenDeviceError,
            TestingSmartCardSimulation::ErrorCessation::kAfterOneError),
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::kOpenDeviceError,
            TestingSmartCardSimulation::ErrorCessation::kAfterTwoErrors),
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::kClaimInterfaceError,
            TestingSmartCardSimulation::ErrorCessation::kAfterOneError),
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::kClaimInterfaceError,
            TestingSmartCardSimulation::ErrorCessation::kAfterTwoErrors),
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::kBulkTransferError,
            TestingSmartCardSimulation::ErrorCessation::kAfterReset),
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::
                kBulkTransferUnrelatedReplies,
            TestingSmartCardSimulation::ErrorCessation::kAfterOneError),
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::
                kBulkTransferUnrelatedReplies,
            TestingSmartCardSimulation::ErrorCessation::kAfterTwoErrors),
        TransientErrorTestParam(
            TestingSmartCardSimulation::ErrorMode::
                kBulkTransferUnrelatedReplies,
            TestingSmartCardSimulation::ErrorCessation::kAfterReset)));

// Reader initialization succeeds after retrying from temporary USB errors.
TEST_P(SmartCardConnectorApplicationReaderTransientErrorTest,
       ReaderSuccessAfterInitialError) {
  // Arrange:
  StartApplication();
  SetUpJsClient();
  SetUpSCardContext();

  // Act: simulate a reader with a temporary error.
  TestingSmartCardSimulation::Device device;
  device.id = 123;
  device.type = TestingSmartCardSimulation::DeviceType::kGemaltoPcTwinReader;
  device.error_mode = GetParam().error_mode;
  device.error_cessation = GetParam().error_cessation;
  SetUsbDevices({device});

  // Assert: eventually the error notifications stop occurring and the reader
  // gets successfully initialized.
  do {
  } while (reader_notification_observer().WaitAndPop() !=
           "reader_finish_add:Gemalto PC Twin Reader");
  std::vector<std::string> readers;
  EXPECT_EQ(SimulateListReadersCallFromJsClient(kFakeHandlerId, scard_context(),
                                                /*groups=*/Value(), readers),
            SCARD_S_SUCCESS);
  EXPECT_THAT(readers, ElementsAre(kGemaltoPcTwinReaderPcscName0));
}

}  // namespace google_smart_card
