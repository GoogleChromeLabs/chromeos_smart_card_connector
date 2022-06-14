// Copyright 2016 Google Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include "libusb_js_proxy.h"

#include <stdint.h>
#include <stdlib.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <libusb.h>

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/value.h>

#include "common/cpp/src/public/testing_global_context.h"
#include "common/cpp/src/public/value_builder.h"

#ifdef __native_client__
// Native Client's version of Google Test uses a different name of the macro for
// parameterized tests.
#define INSTANTIATE_TEST_SUITE_P INSTANTIATE_TEST_CASE_P
// Native Client's version of Google Mock doesn't provide the C++11 "override"
// specifier for the mock method definitions.
#pragma GCC diagnostic ignored "-Winconsistent-missing-override"
// Native Client's version of Google Test macro INSTANTIATE_TEST_CASE_P
// produces this warning when being used without test generator parameters.
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif  // __native_client__

using testing::_;
using testing::Assign;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::MockFunction;
using testing::WithArgs;

namespace google_smart_card {

class LibusbJsProxyTest : public ::testing::Test {
 protected:
  LibusbJsProxyTest() {
    // Bypass `LibusbJsProxy` asserts that it must not be called from the main
    // thread. These asserts are to prevent deadlocks, because with the real
    // JavaScript counterpart it's impossible to receive the JS response without
    // letting the main thread's event loop pump. In the unit test, it's not a
    // concern.
    global_context_.set_creation_thread_is_event_loop(false);
  }
  ~LibusbJsProxyTest() override = default;

  // A convenience wrapper around `LibusbJsProxy::LibusbGetDeviceList()`.
  std::vector<libusb_device*> GetLibusbDevices() {
    libusb_device** device_list = nullptr;
    ssize_t ret_code =
        libusb_js_proxy_.LibusbGetDeviceList(/*ctx=*/nullptr, &device_list);
    if (ret_code < 0) {
      ADD_FAILURE() << "LibusbGetDeviceList failed with " << ret_code;
      return {};
    }
    std::vector<libusb_device*> devices(device_list, device_list + ret_code);
    if (std::find(devices.begin(), devices.end(), nullptr) != devices.end()) {
      ADD_FAILURE() << "LibusbGetDeviceList returned a null element";
      return {};
    }
    if (device_list[ret_code]) {
      ADD_FAILURE()
          << "LibusbGetDeviceList returned non-null after last element";
      return {};
    }
    libusb_js_proxy_.LibusbFreeDeviceList(device_list, /*unref_devices=*/false);
    return devices;
  }

  void FreeLibusbDevices(const std::vector<libusb_device*>& devices) {
    for (auto* device : devices)
      libusb_js_proxy_.LibusbUnrefDevice(device);
  }

  TypedMessageRouter typed_message_router_;
  TestingGlobalContext global_context_{&typed_message_router_};
  LibusbJsProxy libusb_js_proxy_{&global_context_, &typed_message_router_};
};

// Test `LibusbInit()` and `LibusbExit()`.
TEST_F(LibusbJsProxyTest, ContextsCreation) {
  EXPECT_EQ(libusb_js_proxy_.LibusbInit(nullptr), LIBUSB_SUCCESS);

  // Initializing a default context for the second time doesn't do anything
  EXPECT_EQ(libusb_js_proxy_.LibusbInit(nullptr), LIBUSB_SUCCESS);

  libusb_context* context_1 = nullptr;
  EXPECT_EQ(libusb_js_proxy_.LibusbInit(&context_1), LIBUSB_SUCCESS);
  EXPECT_TRUE(context_1);

  libusb_context* context_2 = nullptr;
  EXPECT_EQ(libusb_js_proxy_.LibusbInit(&context_2), LIBUSB_SUCCESS);
  EXPECT_TRUE(context_2);
  EXPECT_NE(context_1, context_2);

  libusb_js_proxy_.LibusbExit(context_1);
  libusb_js_proxy_.LibusbExit(context_2);
  libusb_js_proxy_.LibusbExit(/*ctx=*/nullptr);
}

// Test `LibusbGetDeviceList()` failure when the JS side returns an error.
TEST_F(LibusbJsProxyTest, DevicesListingWithFailure) {
  // Arrange:
  global_context_.WillReplyToRequestWithError(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray), "fake failure");

  // Act:
  libusb_device** device_list;
  EXPECT_EQ(libusb_js_proxy_.LibusbGetDeviceList(/*ctx=*/nullptr, &device_list),
            LIBUSB_ERROR_OTHER);
}

// Test `LibusbGetDeviceList()` successful scenario with zero readers.
TEST_F(LibusbJsProxyTest, DevicesListingWithNoItems) {
  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices", /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/Value(Value::Type::kArray));

  // Act.
  EXPECT_TRUE(GetLibusbDevices().empty());
}

// Test `LibusbGetDeviceList()` successful scenario with 2 readers.
TEST_F(LibusbJsProxyTest, DevicesListingWithTwoItems) {
  // Arrange.
  Value fake_js_reply = ArrayValueBuilder()
                            .Add(DictValueBuilder()
                                     .Add("deviceId", 0)
                                     .Add("vendorId", 0)
                                     .Add("productId", 0)
                                     .Get())
                            .Add(DictValueBuilder()
                                     .Add("deviceId", 1)
                                     .Add("vendorId", 0)
                                     .Add("productId", 0)
                                     .Get())
                            .Get();
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/std::move(fake_js_reply));

  // Act.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 2U);
  EXPECT_NE(devices[0], devices[1]);

  FreeLibusbDevices(devices);
}

// Test `LibusbFreeDeviceList()` correctly cleans up an empty device list when
// called with `unref_devices`=true.
TEST_F(LibusbJsProxyTest, DevicesListFreeingWithNoItems) {
  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices", /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/Value(Value::Type::kArray));

  // Act.
  libusb_device** device_list = nullptr;
  EXPECT_EQ(libusb_js_proxy_.LibusbGetDeviceList(/*ctx=*/nullptr, &device_list),
            0);
  // The test can't really assert the readers were actually deallocated, but
  // running it under ASan should guarantee catching mistakes.
  libusb_js_proxy_.LibusbFreeDeviceList(device_list, /*unref_devices=*/true);
}

// Test `LibusbFreeDeviceList()` correctly cleans up a list with 2 readers when
// called with `unref_devices`=true.
TEST_F(LibusbJsProxyTest, DevicesListFreeingWithTwoItems) {
  // Arrange.
  Value fake_js_reply = ArrayValueBuilder()
                            .Add(DictValueBuilder()
                                     .Add("deviceId", 0)
                                     .Add("vendorId", 0)
                                     .Add("productId", 0)
                                     .Get())
                            .Add(DictValueBuilder()
                                     .Add("deviceId", 1)
                                     .Add("vendorId", 0)
                                     .Add("productId", 0)
                                     .Get())
                            .Get();
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/std::move(fake_js_reply));

  // Act.
  libusb_device** device_list = nullptr;
  EXPECT_EQ(libusb_js_proxy_.LibusbGetDeviceList(/*ctx=*/nullptr, &device_list),
            2);
  // The test can't really assert the readers were actually deallocated, but
  // running it under ASan should guarantee catching mistakes.
  libusb_js_proxy_.LibusbFreeDeviceList(device_list, /*unref_devices=*/true);
}

// Test `LibusbOpen()` successful scenario.
TEST_F(LibusbJsProxyTest, DeviceOpening) {
  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/
      ArrayValueBuilder()
          .Add(DictValueBuilder()
                   .Add("deviceId", 123)
                   .Add("vendorId", 1)
                   .Add("productId", 2)
                   .Get())
          .Get());
  global_context_.WillReplyToRequestWith(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*result_to_reply_with=*/Value(456));
  global_context_.WillReplyToRequestWith(
      "libusb", "closeDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Add(456).Get(),
      /*result_to_reply_with=*/Value());

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);
  // Connect to the device.
  libusb_device_handle* device_handle = nullptr;
  ASSERT_EQ(libusb_js_proxy_.LibusbOpen(devices[0], &device_handle),
            LIBUSB_SUCCESS);
  ASSERT_TRUE(device_handle);
  // Disconnect from the device.
  libusb_js_proxy_.LibusbClose(device_handle);

  FreeLibusbDevices(devices);
}

// Test `LibusbOpen()` failure when the JS side returns an error.
TEST_F(LibusbJsProxyTest, DeviceOpeningFailure) {
  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/
      ArrayValueBuilder()
          .Add(DictValueBuilder()
                   .Add("deviceId", 123)
                   .Add("vendorId", 1)
                   .Add("productId", 2)
                   .Get())
          .Get());
  global_context_.WillReplyToRequestWithError(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*error_to_reply_with=*/"fake error");

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);
  // Connect to the device.
  libusb_device_handle* device_handle = nullptr;
  EXPECT_EQ(libusb_js_proxy_.LibusbOpen(devices[0], &device_handle),
            LIBUSB_ERROR_OTHER);

  FreeLibusbDevices(devices);
}

// Test `LibusbClose()` doesn't crash when the JavaScript side reports an error.
TEST_F(LibusbJsProxyTest, DeviceClosingFailure) {
  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/
      ArrayValueBuilder()
          .Add(DictValueBuilder()
                   .Add("deviceId", 123)
                   .Add("vendorId", 1)
                   .Add("productId", 2)
                   .Get())
          .Get());
  global_context_.WillReplyToRequestWith(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*result_to_reply_with=*/Value(456));
  global_context_.WillReplyToRequestWithError(
      "libusb", "closeDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Add(456).Get(),
      /*error_to_reply_with=*/"fake error");

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);
  // Connect to the device.
  libusb_device_handle* device_handle = nullptr;
  ASSERT_EQ(libusb_js_proxy_.LibusbOpen(devices[0], &device_handle),
            LIBUSB_SUCCESS);
  ASSERT_TRUE(device_handle);
  // Disconnect from the device. The `LibusbClose()` function is void, and we
  // expect it to not crash despite the error simulated on the JS side.
  libusb_js_proxy_.LibusbClose(device_handle);

  FreeLibusbDevices(devices);
}

// Test `LibusbRefDevice()` and `LibusbUnrefDevice()` that increment and
// decrement the libusb_device reference counter.
TEST_F(LibusbJsProxyTest, DeviceRefUnref) {
  constexpr int kIterationCount = 100;

  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/
      ArrayValueBuilder()
          .Add(DictValueBuilder()
                   .Add("deviceId", 1)
                   .Add("vendorId", 2)
                   .Add("productId", 3)
                   .Get())
          .Get());

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);
  // Increment and decrement the device's reference counter. The device object
  // should stay valid (note that the test can't assert this explicitly, but
  // running it under ASan should ensure that).
  EXPECT_EQ(libusb_js_proxy_.LibusbRefDevice(devices[0]), devices[0]);
  libusb_js_proxy_.LibusbUnrefDevice(devices[0]);
  // Increase and then decrease the reference counter by `kIterationCount`. Same
  // as above, we can't assert anything explicitly here.
  for (int i = 0; i < kIterationCount; ++i)
    EXPECT_EQ(libusb_js_proxy_.LibusbRefDevice(devices[0]), devices[0]);
  for (int i = 0; i < kIterationCount; ++i)
    libusb_js_proxy_.LibusbUnrefDevice(devices[0]);

  FreeLibusbDevices(devices);
}

// Test fixture that simulates a single USB device present, and automatically
// connects to the device in SetUp.
class LibusbJsProxyWithDeviceTest : public LibusbJsProxyTest {
 protected:
  static constexpr int kJsDeviceId = 123;
  static constexpr int kJsDeviceHandle = 456;

  LibusbJsProxyWithDeviceTest() = default;
  ~LibusbJsProxyWithDeviceTest() = default;

  void SetUp() override {
    ASSERT_EQ(libusb_js_proxy_.LibusbInit(/*ctx=*/nullptr), LIBUSB_SUCCESS);

    // Obtain the libusb device.
    global_context_.WillReplyToRequestWith(
        "libusb", "listDevices",
        /*arguments=*/Value(Value::Type::kArray),
        /*result_to_reply_with=*/
        ArrayValueBuilder()
            .Add(DictValueBuilder()
                     .Add("deviceId", kJsDeviceId)
                     .Add("vendorId", 2)
                     .Add("productId", 3)
                     .Get())
            .Get());
    std::vector<libusb_device*> devices = GetLibusbDevices();
    ASSERT_EQ(devices.size(), 1U);
    device_ = devices[0];

    // Connect to the device.
    global_context_.WillReplyToRequestWith(
        "libusb", "openDeviceHandle",
        /*arguments=*/ArrayValueBuilder().Add(kJsDeviceId).Get(),
        /*result_to_reply_with=*/Value(kJsDeviceHandle));
    EXPECT_EQ(libusb_js_proxy_.LibusbOpen(device_, &device_handle_),
              LIBUSB_SUCCESS);
    EXPECT_TRUE(device_handle_);
  }

  void TearDown() override {
    // Close the libusb device handle, which triggers a call to JS.
    global_context_.WillReplyToRequestWith(
        "libusb", "closeDeviceHandle",
        /*arguments=*/
        ArrayValueBuilder().Add(kJsDeviceId).Add(kJsDeviceHandle).Get(),
        /*result_to_reply_with=*/Value());
    libusb_js_proxy_.LibusbClose(device_handle_);
    device_handle_ = nullptr;

    // Deallocate the libusb device.
    libusb_js_proxy_.LibusbUnrefDevice(device_);
    device_ = nullptr;

    // Free the libusb global state.
    libusb_js_proxy_.LibusbExit(/*ctx=*/nullptr);
  }

  libusb_device* device_ = nullptr;
  libusb_device_handle* device_handle_ = nullptr;
};

// Test `LibusbResetDevice()` successful scenario.
TEST_F(LibusbJsProxyWithDeviceTest, DeviceResetting) {
  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "resetDevice",
      /*arguments=*/
      ArrayValueBuilder().Add(kJsDeviceId).Add(kJsDeviceHandle).Get(),
      /*result_to_reply_with=*/Value());

  // Act.
  EXPECT_EQ(libusb_js_proxy_.LibusbResetDevice(device_handle_), LIBUSB_SUCCESS);
}

// Test `LibusbResetDevice()` failure due to the JS call returning an error.
TEST_F(LibusbJsProxyWithDeviceTest, DeviceResettingFailure) {
  // Arrange.
  global_context_.WillReplyToRequestWithError(
      "libusb", "resetDevice",
      /*arguments=*/
      ArrayValueBuilder().Add(kJsDeviceId).Add(kJsDeviceHandle).Get(),
      /*error_to_reply_with=*/"fake error");

  // Act.
  EXPECT_EQ(libusb_js_proxy_.LibusbResetDevice(device_handle_),
            LIBUSB_ERROR_OTHER);
}

// Tests `LibusbControlTransfer()` successful scenario when sending data to the
// output endpoint.
TEST_F(LibusbJsProxyWithDeviceTest, OutputControlTransfer) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  // non-const, as LibusbControlTransfer() takes a non-const pointer to it -
  // following libusb's original interface.
  std::vector<uint8_t> data = {1, 2, 3};

  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("dataToSend", data)
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Get())
          .Get(),
      /*result_to_reply_with=*/Value(Value::Type::kDictionary));

  // Act.
  EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                    LIBUSB_ENDPOINT_OUT,
                kTransferRequest, kTransferValue, kTransferIndex, &data[0],
                data.size(), /*timeout=*/100),
            static_cast<int>(data.size()));
}

// Test `LibusbControlTransfer()` failure scenario due to a JS error during an
// output transfer.
TEST_F(LibusbJsProxyWithDeviceTest, OutputControlTransferFailure) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  // non-const, as LibusbControlTransfer() takes a non-const pointer to it -
  // following libusb's original interface.
  std::vector<uint8_t> data = {1, 2, 3};

  // Arrange.
  global_context_.WillReplyToRequestWithError(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("dataToSend", data)
                   .Add("index", kTransferIndex)
                   .Add("recipient", "interface")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "class")
                   .Add("value", kTransferValue)
                   .Get())
          .Get(),
      /*error_to_reply_with=*/"fake error");

  // Act.
  EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_INTERFACE | LIBUSB_REQUEST_TYPE_CLASS |
                    LIBUSB_ENDPOINT_OUT,
                kTransferRequest, kTransferValue, kTransferIndex, &data[0],
                data.size(), /*timeout=*/100),
            LIBUSB_ERROR_OTHER);
}

// Tests `LibusbControlTransfer()` successful scenario when reading data from
// an endpoint.
TEST_F(LibusbJsProxyWithDeviceTest, InputControlTransfer) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Add("lengthToReceive", kData.size())
                   .Get())
          .Get(),
      /*result_to_reply_with=*/
      DictValueBuilder().Add("receivedData", kData).Get());

  // Act.
  std::vector<uint8_t> received_data(kData.size());
  EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                    LIBUSB_ENDPOINT_IN,
                kTransferRequest, kTransferValue, kTransferIndex,
                &received_data[0], received_data.size(), /*timeout=*/100),
            static_cast<int>(kData.size()));
  EXPECT_EQ(received_data, kData);
}

// Tests `LibusbControlTransfer()` scenario when the data read from an endpoint
// turned out to be shorter than requested.
TEST_F(LibusbJsProxyWithDeviceTest, InputControlTransferShorterData) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  constexpr int kDataLengthRequested = 100;
  const std::vector<uint8_t> kDataResponded = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Add("lengthToReceive", kDataLengthRequested)
                   .Get())
          .Get(),
      /*result_to_reply_with=*/
      DictValueBuilder().Add("receivedData", kDataResponded).Get());

  // Act.
  std::vector<uint8_t> received_data(kDataLengthRequested);
  EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                    LIBUSB_ENDPOINT_IN,
                kTransferRequest, kTransferValue, kTransferIndex,
                &received_data[0], received_data.size(), /*timeout=*/100),
            static_cast<int>(kDataResponded.size()));
  EXPECT_EQ(std::vector<uint8_t>(received_data.begin(),
                                 received_data.begin() + kDataResponded.size()),
            kDataResponded);
}

// Tests `LibusbControlTransfer()` failure scenario when JS input transfer
// returned an error.
TEST_F(LibusbJsProxyWithDeviceTest, InputControlTransferFailure) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  constexpr int kDataLengthRequested = 100;

  // Arrange.
  global_context_.WillReplyToRequestWithError(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "device")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "vendor")
                   .Add("value", kTransferValue)
                   .Add("lengthToReceive", kDataLengthRequested)
                   .Get())
          .Get(),
      /*error_to_reply_with=*/"fake error");

  // Act.
  std::vector<uint8_t> received_data(kDataLengthRequested);
  EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_VENDOR |
                    LIBUSB_ENDPOINT_IN,
                kTransferRequest, kTransferValue, kTransferIndex,
                &received_data[0], received_data.size(), /*timeout=*/100),
            LIBUSB_ERROR_OTHER);
}

// Tests `LibusbControlTransfer()` timeout scenario for an input transfer.
TEST_F(LibusbJsProxyWithDeviceTest, InputControlTransferTimeout) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  constexpr int kDataLengthRequested = 100;

  // Arrange. Set up the expectation for the request message. We won't reply to
  // this message.
  auto waiter = global_context_.CreateRequestWaiter(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "device")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "vendor")
                   .Add("value", kTransferValue)
                   .Add("lengthToReceive", kDataLengthRequested)
                   .Get())
          .Get());

  // Act. This call will block for about a second before returning (we don't
  // verify the clocks to avoid flakiness).
  std::vector<uint8_t> received_data(kDataLengthRequested);
  EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_VENDOR |
                    LIBUSB_ENDPOINT_IN,
                kTransferRequest, kTransferValue, kTransferIndex,
                &received_data[0], received_data.size(), /*timeout=*/1000),
            LIBUSB_ERROR_OTHER);
}

// Tests `LibusbControlTransfer()` timeout scenario for an output transfer.
TEST_F(LibusbJsProxyWithDeviceTest, OutputControlTransferTimeout) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  // non-const, as LibusbControlTransfer() takes a non-const pointer to it -
  // following libusb's original interface.
  std::vector<uint8_t> data = {1, 2, 3};

  // Arrange. Set up the expectation for the request message. We won't reply to
  // this message.
  auto waiter = global_context_.CreateRequestWaiter(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("dataToSend", data)
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Get())
          .Get());

  // Act. This call will block for about a second before returning (we don't
  // verify the clocks to avoid flakiness).
  EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                    LIBUSB_ENDPOINT_OUT,
                kTransferRequest, kTransferValue, kTransferIndex, &data[0],
                data.size(), /*timeout=*/1000),
            LIBUSB_ERROR_OTHER);
}

// Test the correctness of work of multiple threads issuing a sequence of
// synchronous transfer requests. It's a regression test for #464 and #465.
//
// Each transfer request is resolved immediately on the same thread that
// initiated the transfer.
TEST_F(LibusbJsProxyWithDeviceTest, ControlTransfersMultiThreadedStressTest) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};
  constexpr int kThreadCount = 10;
  // A high number of transfers increases the chances of catching a bug, but the
  // constant is lower in the Debug mode to avoid running too long.
  constexpr int kIterationsPerThread =
#ifdef NDEBUG
      1000
#else
      100
#endif
      ;

  // Arrange.
  for (int i = 0; i < kThreadCount * kIterationsPerThread; ++i) {
    // Each test thread iteration consists of one input and one output transfer
    // - prepare replies for them in advance.
    global_context_.WillReplyToRequestWith(
        "libusb", "controlTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(kJsDeviceId)
            .Add(kJsDeviceHandle)
            .Add(DictValueBuilder()
                     .Add("index", kTransferIndex)
                     .Add("recipient", "endpoint")
                     .Add("request", kTransferRequest)
                     .Add("requestType", "standard")
                     .Add("value", kTransferValue)
                     .Add("lengthToReceive", kData.size())
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder().Add("receivedData", kData).Get());
    global_context_.WillReplyToRequestWith(
        "libusb", "controlTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(kJsDeviceId)
            .Add(kJsDeviceHandle)
            .Add(DictValueBuilder()
                     .Add("dataToSend", kData)
                     .Add("index", kTransferIndex)
                     .Add("recipient", "endpoint")
                     .Add("request", kTransferRequest)
                     .Add("requestType", "standard")
                     .Add("value", kTransferValue)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
  }

  // Act.
  std::vector<std::thread> threads;
  for (int thread_index = 0; thread_index < kThreadCount; ++thread_index) {
    threads.emplace_back([this, kData] {
      for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
        // Test input transfer.
        std::vector<uint8_t> received_data(kData.size());
        EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                      device_handle_,
                      LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                          LIBUSB_ENDPOINT_IN,
                      kTransferRequest, kTransferValue, kTransferIndex,
                      &received_data[0], received_data.size(), /*timeout=*/0),
                  static_cast<int>(kData.size()));
        EXPECT_EQ(received_data, kData);
        // Test output transfer.
        std::vector<uint8_t> data = kData;
        EXPECT_EQ(libusb_js_proxy_.LibusbControlTransfer(
                      device_handle_,
                      LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                          LIBUSB_ENDPOINT_OUT,
                      kTransferRequest, kTransferValue, kTransferIndex,
                      &data[0], data.size(), /*timeout=*/0),
                  static_cast<int>(data.size()));
      }
    });
  }
  for (size_t thread_index = 0; thread_index < kThreadCount; ++thread_index)
    threads[thread_index].join();
}

// Test an asynchronous input control transfer successful scenario.
TEST_F(LibusbJsProxyWithDeviceTest, AsyncInputControlTransfer) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Add("lengthToReceive", kData.size())
                   .Get())
          .Get(),
      /*result_to_reply_with=*/
      DictValueBuilder().Add("receivedData", kData).Get());
  auto transfer_callback = [](libusb_transfer* transfer) {
    ASSERT_TRUE(transfer);
    // `user_data` points to `transfer_completed` (a captureless lambda has no
    // other way of telling the test it's run).
    *static_cast<bool*>(transfer->user_data) = true;
  };

  // Act.
  std::vector<uint8_t> setup(LIBUSB_CONTROL_SETUP_SIZE + kData.size());
  libusb_fill_control_setup(
      setup.data(),
      LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
          LIBUSB_ENDPOINT_IN,
      kTransferRequest, kTransferValue, kTransferIndex, kData.size());

  libusb_transfer* const transfer =
      libusb_js_proxy_.LibusbAllocTransfer(/*iso_packets=*/0);
  ASSERT_TRUE(transfer);
  bool transfer_completed = false;
  libusb_fill_control_transfer(
      transfer, device_handle_, setup.data(), transfer_callback,
      /*user_data=*/static_cast<void*>(&transfer_completed),
      /*timeout=*/0);

  EXPECT_EQ(libusb_js_proxy_.LibusbSubmitTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completed);
  // Let the fake JS result propagate.
  do {
    EXPECT_EQ(libusb_js_proxy_.LibusbHandleEvents(/*ctx=*/nullptr),
              LIBUSB_SUCCESS);
  } while (!transfer_completed);

  // Assert.
  EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_COMPLETED);
  EXPECT_EQ(transfer->actual_length, static_cast<int>(kData.size()));
  EXPECT_EQ(std::vector<uint8_t>(setup.begin() + LIBUSB_CONTROL_SETUP_SIZE,
                                 setup.end()),
            kData);
  // Attempting to cancel a completed transfer fails.
  EXPECT_NE(libusb_js_proxy_.LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);

  // Cleanup:
  libusb_js_proxy_.LibusbFreeTransfer(transfer);
}

// Test the cancellation of an asynchronous input control transfer.
//
// Other slight variations of this test as opposed to the above include using
// the `LIBUSB_TRANSFER_FREE_TRANSFER` flag and `LibusbHandleEventsCompleted()`.
TEST_F(LibusbJsProxyWithDeviceTest, AsyncInputControlTransferCancellation) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  constexpr int kDataLengthRequested = 100;

  // Arrange. Set up the expectation for the request message. We won't reply to
  // this message (until after we cancel the transfer).
  auto waiter = global_context_.CreateRequestWaiter(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Add("lengthToReceive", kDataLengthRequested)
                   .Get())
          .Get());
  auto transfer_callback = [](libusb_transfer* transfer) {
    ASSERT_TRUE(transfer);
    EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_CANCELLED);
    EXPECT_EQ(transfer->actual_length, 0);
    // `user_data` points to `transfer_completed` (a captureless lambda has no
    // other way of telling the test it's run).
    *static_cast<int*>(transfer->user_data) = true;
  };

  // Act.
  std::vector<uint8_t> setup(LIBUSB_CONTROL_SETUP_SIZE + kDataLengthRequested);
  libusb_fill_control_setup(
      setup.data(),
      LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
          LIBUSB_ENDPOINT_IN,
      kTransferRequest, kTransferValue, kTransferIndex, kDataLengthRequested);

  libusb_transfer* const transfer =
      libusb_js_proxy_.LibusbAllocTransfer(/*iso_packets=*/0);
  ASSERT_TRUE(transfer);
  int transfer_completed = 0;
  libusb_fill_control_transfer(
      transfer, device_handle_, setup.data(), transfer_callback,
      /*user_data=*/static_cast<void*>(&transfer_completed),
      /*timeout=*/0);
  // In this test we also verify the automatic deallocation of the transfer.
  transfer->flags = LIBUSB_TRANSFER_FREE_TRANSFER;

  EXPECT_EQ(libusb_js_proxy_.LibusbSubmitTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completed);

  EXPECT_EQ(libusb_js_proxy_.LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completed);
  // Second attempt to cancel a transfer fails.
  EXPECT_NE(libusb_js_proxy_.LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completed);
  // Let the cancellation propagate.
  do {
    EXPECT_EQ(libusb_js_proxy_.LibusbHandleEventsCompleted(/*ctx=*/nullptr,
                                                           &transfer_completed),
              LIBUSB_SUCCESS);
  } while (!transfer_completed);

  // A reply from the JS side has no effect for the already canceled transfer.
  waiter->Reply(/*result_to_reply_with=*/DictValueBuilder()
                    .Add("receivedData", std::vector<uint8_t>({1, 2, 3}))
                    .Get());

  // Nothing to assert here - due to the `LIBUSB_TRANSFER_FREE_TRANSFER` flag
  // the `transfer` is already deallocated here. All assertions are done inside
  // the callback.
}

// Test an asynchronous output control transfer successful scenario.
TEST_F(LibusbJsProxyWithDeviceTest, AsyncOutputControlTransfer) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context_.WillReplyToRequestWith(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Add("dataToSend", kData)
                   .Get())
          .Get(),
      /*result_to_reply_with=*/Value(Value::Type::kDictionary));
  auto transfer_callback = [](libusb_transfer* transfer) {
    ASSERT_TRUE(transfer);
    // `user_data` points to `transfer_completed` (a captureless lambda has no
    // other way of telling the test it's run).
    *static_cast<bool*>(transfer->user_data) = true;
  };

  // Act.
  std::vector<uint8_t> setup(LIBUSB_CONTROL_SETUP_SIZE + kData.size());
  libusb_fill_control_setup(
      setup.data(),
      LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
          LIBUSB_ENDPOINT_OUT,
      kTransferRequest, kTransferValue, kTransferIndex, kData.size());
  std::copy(kData.begin(), kData.end(),
            setup.begin() + LIBUSB_CONTROL_SETUP_SIZE);

  libusb_transfer* const transfer =
      libusb_js_proxy_.LibusbAllocTransfer(/*iso_packets=*/0);
  ASSERT_TRUE(transfer);
  bool transfer_completed = false;
  libusb_fill_control_transfer(
      transfer, device_handle_, setup.data(), transfer_callback,
      /*user_data=*/static_cast<void*>(&transfer_completed),
      /*timeout=*/0);

  EXPECT_EQ(libusb_js_proxy_.LibusbSubmitTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completed);
  // Let the fake JS result propagate.
  do {
    EXPECT_EQ(libusb_js_proxy_.LibusbHandleEvents(/*ctx=*/nullptr),
              LIBUSB_SUCCESS);
  } while (!transfer_completed);

  // Assert.
  EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_COMPLETED);
  EXPECT_EQ(transfer->actual_length, static_cast<int>(kData.size()));
  // Attempting to cancel a completed transfer fails.
  EXPECT_NE(libusb_js_proxy_.LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);

  // Cleanup:
  libusb_js_proxy_.LibusbFreeTransfer(transfer);
}

// Test that it's not possible to cancel an asynchronous output control transfer
// (only cancelling input transfers is supported by our implementation).
TEST_F(LibusbJsProxyWithDeviceTest, AsyncOutputControlTransferCancellation) {
  constexpr int kTransferRequest = 1;
  constexpr int kTransferIndex = 24;
  constexpr int kTransferValue = 42;
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange. Set up the expectation for the request message. We will reply to
  // this message only after attempting to cancel the transfer.
  auto waiter = global_context_.CreateRequestWaiter(
      "libusb", "controlTransfer",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(DictValueBuilder()
                   .Add("index", kTransferIndex)
                   .Add("recipient", "endpoint")
                   .Add("request", kTransferRequest)
                   .Add("requestType", "standard")
                   .Add("value", kTransferValue)
                   .Add("dataToSend", kData)
                   .Get())
          .Get());
  auto transfer_callback = [](libusb_transfer* transfer) {
    ASSERT_TRUE(transfer);
    EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_COMPLETED);
    // Check `actual_length` equals `kData.size()` (we can't pass it explicitly
    // as we're a captureless lambda).
    EXPECT_EQ(transfer->actual_length, 6);

    // `user_data` points to `transfer_completed` (a captureless lambda has no
    // other way of telling the test it's run).
    *static_cast<bool*>(transfer->user_data) = true;
  };

  // Act.
  std::vector<uint8_t> setup(LIBUSB_CONTROL_SETUP_SIZE + kData.size());
  libusb_fill_control_setup(
      setup.data(),
      LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
          LIBUSB_ENDPOINT_OUT,
      kTransferRequest, kTransferValue, kTransferIndex, kData.size());
  std::copy(kData.begin(), kData.end(),
            setup.begin() + LIBUSB_CONTROL_SETUP_SIZE);

  libusb_transfer* const transfer =
      libusb_js_proxy_.LibusbAllocTransfer(/*iso_packets=*/0);
  ASSERT_TRUE(transfer);
  bool transfer_completed = false;
  libusb_fill_control_transfer(
      transfer, device_handle_, setup.data(), transfer_callback,
      /*user_data=*/static_cast<void*>(&transfer_completed),
      /*timeout=*/0);
  // In this test we also verify the automatic deallocation of the transfer.
  transfer->flags = LIBUSB_TRANSFER_FREE_TRANSFER;

  EXPECT_EQ(libusb_js_proxy_.LibusbSubmitTransfer(transfer), LIBUSB_SUCCESS);

  // Wait for the JS request to be sent.
  waiter->Wait();
  EXPECT_FALSE(transfer_completed);

  // Attempt to cancel the transfer - this is expected to fail.
  EXPECT_EQ(libusb_js_proxy_.LibusbCancelTransfer(transfer),
            LIBUSB_ERROR_NOT_FOUND);

  // Simulate a successful transfer reply from the JS side.
  waiter->Reply(/*result_to_reply_with=*/Value(Value::Type::kDictionary));
  EXPECT_FALSE(transfer_completed);

  // Let the fake JS result propagate.
  do {
    EXPECT_EQ(libusb_js_proxy_.LibusbHandleEvents(/*ctx=*/nullptr),
              LIBUSB_SUCCESS);
  } while (!transfer_completed);

  // Nothing to assert here - due to the `LIBUSB_TRANSFER_FREE_TRANSFER` flag
  // the `transfer` is already deallocated here. All assertions are done inside
  // the callback.
}

// TODO(#429): Resurrect the tests by reimplementing them on top of the
// libusb-to-JS adaptor instead of the chrome_usb::ApiBridge.
#if 0

namespace {

constexpr char kBoolValues[] = {false, true};

class MockChromeUsbApiBridge final : public chrome_usb::ApiBridgeInterface {
 public:
  MOCK_METHOD1(GetDevices,
               RequestResult<chrome_usb::GetDevicesResult>(
                   const chrome_usb::GetDevicesOptions& options));

  MOCK_METHOD1(GetUserSelectedDevices,
               RequestResult<chrome_usb::GetUserSelectedDevicesResult>(
                   const chrome_usb::GetUserSelectedDevicesOptions& options));

  MOCK_METHOD1(GetConfigurations,
               RequestResult<chrome_usb::GetConfigurationsResult>(
                   const chrome_usb::Device& device));

  MOCK_METHOD1(OpenDevice,
               RequestResult<chrome_usb::OpenDeviceResult>(
                   const chrome_usb::Device& device));

  MOCK_METHOD1(CloseDevice,
               RequestResult<chrome_usb::CloseDeviceResult>(
                   const chrome_usb::ConnectionHandle& connection_handle));

  MOCK_METHOD2(SetConfiguration,
               RequestResult<chrome_usb::SetConfigurationResult>(
                   const chrome_usb::ConnectionHandle& connection_handle,
                   int64_t configuration_value));

  MOCK_METHOD1(GetConfiguration,
               RequestResult<chrome_usb::GetConfigurationResult>(
                   const chrome_usb::ConnectionHandle& connection_handle));

  MOCK_METHOD1(ListInterfaces,
               RequestResult<chrome_usb::ListInterfacesResult>(
                   const chrome_usb::ConnectionHandle& connection_handle));

  MOCK_METHOD2(ClaimInterface,
               RequestResult<chrome_usb::ClaimInterfaceResult>(
                   const chrome_usb::ConnectionHandle& connection_handle,
                   int64_t interface_number));

  MOCK_METHOD2(ReleaseInterface,
               RequestResult<chrome_usb::ReleaseInterfaceResult>(
                   const chrome_usb::ConnectionHandle& connection_handle,
                   int64_t interface_number));

  MOCK_METHOD3(AsyncControlTransfer,
               void(const chrome_usb::ConnectionHandle& connection_handle,
                    const chrome_usb::ControlTransferInfo& transfer_info,
                    chrome_usb::AsyncTransferCallback callback));

  MOCK_METHOD3(AsyncBulkTransfer,
               void(const chrome_usb::ConnectionHandle& connection_handle,
                    const chrome_usb::GenericTransferInfo& transfer_info,
                    chrome_usb::AsyncTransferCallback callback));

  MOCK_METHOD3(AsyncInterruptTransfer,
               void(const chrome_usb::ConnectionHandle& connection_handle,
                    const chrome_usb::GenericTransferInfo& transfer_info,
                    chrome_usb::AsyncTransferCallback callback));

  MOCK_METHOD1(ResetDevice,
               RequestResult<chrome_usb::ResetDeviceResult>(
                   const chrome_usb::ConnectionHandle& connection_handle));
};

}  // namespace

// TODO(emaxx): Add a test on obtaining device descriptors and attributes

namespace {

class LibusbJsProxyTransfersTest : public LibusbJsProxyWithFakeDeviceTest {
 protected:
  void SetUpMockForSyncControlTransfer(size_t transfer_index, bool is_output) {
    EXPECT_CALL(*chrome_usb_api_bridge,
                AsyncControlTransfer(
                    chrome_usb_connection_handle,
                    GenerateControlTransferInfo(transfer_index, is_output), _))
        .WillOnce(
            WithArgs<2>(Invoke([=](chrome_usb::AsyncTransferCallback callback) {
              callback(
                  GenerateTransferRequestResult(transfer_index, is_output));
            })));
  }

  std::function<void()> SetUpMockForAsyncControlTransfer(size_t transfer_index,
                                                         bool is_output) {
    auto callback_holder =
        std::make_shared<chrome_usb::AsyncTransferCallback>();
    EXPECT_CALL(*chrome_usb_api_bridge,
                AsyncControlTransfer(
                    chrome_usb_connection_handle,
                    GenerateControlTransferInfo(transfer_index, is_output), _))
        .WillOnce(
            Invoke([callback_holder](
                       const chrome_usb::ConnectionHandle&,
                       const chrome_usb::ControlTransferInfo&,
                       chrome_usb::AsyncTransferCallback callback) mutable {
              *callback_holder = callback;
            }))
        .RetiresOnSaturation();
    return [this, transfer_index, is_output, callback_holder]() {
      GOOGLE_SMART_CARD_CHECK(callback_holder);
      (*callback_holder)(
          GenerateTransferRequestResult(transfer_index, is_output));
    };
  }

  void TestSyncControlTransfer(size_t transfer_index, bool is_output) {
    std::vector<uint8_t> data;
    if (is_output)
      data = GenerateTransferData(transfer_index, true);
    else
      data.resize(GenerateTransferData(transfer_index, false).size());

    const int return_code = libusb_js_proxy->LibusbControlTransfer(
        device_handle,
        LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
            (is_output ? LIBUSB_ENDPOINT_OUT : LIBUSB_ENDPOINT_IN),
        kTransferRequestField, kTransferValueField, transfer_index,
        data.empty() ? nullptr : &data[0], data.size(), kTransferTimeout);

    if (IsTransferToFail(transfer_index) ||
        IsTransferToFinishUnsuccessfully(transfer_index)) {
      EXPECT_EQ(LIBUSB_ERROR_OTHER, return_code);
    } else {
      EXPECT_EQ(static_cast<int>(data.size()), return_code);
      if (!is_output)
        EXPECT_EQ(GenerateTransferData(transfer_index, false), data);
    }
  }

  libusb_transfer* StartAsyncControlTransfer(
      size_t transfer_index,
      bool is_output,
      MockFunction<void(libusb_transfer_status)>* transfer_callback) {
    const std::vector<uint8_t> actual_data =
        GenerateTransferData(transfer_index, is_output);
    const size_t buffer_size = LIBUSB_CONTROL_SETUP_SIZE + actual_data.size();
    uint8_t* const buffer = static_cast<uint8_t*>(::malloc(buffer_size));
    if (is_output) {
      std::copy(actual_data.begin(), actual_data.end(),
                buffer + LIBUSB_CONTROL_SETUP_SIZE);
    }
    libusb_fill_control_setup(
        buffer,
        LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
            (is_output ? LIBUSB_ENDPOINT_OUT : LIBUSB_ENDPOINT_IN),
        kTransferRequestField, kTransferValueField, transfer_index,
        actual_data.size());

    libusb_transfer* const transfer = libusb_js_proxy->LibusbAllocTransfer(0);
    libusb_fill_control_transfer(
        transfer, device_handle, buffer,
        &AsyncTransferCallbackWrapper::Callback,
        new AsyncTransferCallbackWrapper(this, transfer_index, is_output,
                                         transfer_callback),
        kTransferTimeout);
    transfer->flags =
        LIBUSB_TRANSFER_FREE_TRANSFER | LIBUSB_TRANSFER_FREE_BUFFER;

    EXPECT_EQ(LIBUSB_SUCCESS, libusb_js_proxy->LibusbSubmitTransfer(transfer));

    return transfer;
  }

  static void SetUpTransferCallbackMockExpectations(
      size_t transfer_index,
      bool is_canceled,
      MockFunction<void(libusb_transfer_status)>* transfer_callback) {
    EXPECT_CALL(*transfer_callback,
                Call(GetExpectedTransferStatus(transfer_index, is_canceled)));
  }

  static void SetUpTransferCallbackMockExpectations(
      size_t transfer_index,
      bool is_canceled,
      MockFunction<void(libusb_transfer_status)>* transfer_callback,
      int* completed) {
    EXPECT_CALL(*transfer_callback,
                Call(GetExpectedTransferStatus(transfer_index, is_canceled)))
        .WillOnce(Assign(completed, 1));
  }

  static bool IsTransferToFail(size_t transfer_index) {
    return transfer_index % 3 == 0;
  }

  static bool IsTransferToFinishUnsuccessfully(size_t transfer_index) {
    return !IsTransferToFail(transfer_index) && transfer_index % 5 == 0;
  }

 private:
  const uint8_t kTransferRequestField = 1;
  const uint16_t kTransferValueField = 2;
  const unsigned kTransferTimeout = 3;

  class AsyncTransferCallbackWrapper {
   public:
    AsyncTransferCallbackWrapper(
        LibusbJsProxyTransfersTest* test_instance,
        size_t transfer_index,
        bool is_output,
        MockFunction<void(libusb_transfer_status)>* transfer_callback)
        : test_instance_(test_instance),
          transfer_index_(transfer_index),
          is_output_(is_output),
          transfer_callback_(transfer_callback) {}

    static void Callback(libusb_transfer* transfer) {
      GOOGLE_SMART_CARD_CHECK(transfer);
      AsyncTransferCallbackWrapper* const instance =
          static_cast<AsyncTransferCallbackWrapper*>(transfer->user_data);

      LibusbJsProxyTransfersTest* const test_instance =
          instance->test_instance_;
      const size_t transfer_index = instance->transfer_index_;
      const bool is_output = instance->is_output_;
      MockFunction<void(libusb_transfer_status)>* const transfer_callback =
          instance->transfer_callback_;

      delete instance;

      test_instance->OnAsyncControlTransferFinished(
          transfer, transfer_index, is_output, transfer_callback);
    }

   private:
    LibusbJsProxyTransfersTest* test_instance_;
    size_t transfer_index_;
    bool is_output_;
    MockFunction<void(libusb_transfer_status)>* transfer_callback_;
  };

  chrome_usb::ControlTransferInfo GenerateControlTransferInfo(
      size_t transfer_index,
      bool is_output) {
    chrome_usb::ControlTransferInfo transfer_info;
    transfer_info.direction =
        is_output ? chrome_usb::Direction::kOut : chrome_usb::Direction::kIn;
    transfer_info.recipient =
        chrome_usb::ControlTransferInfoRecipient::kEndpoint;
    transfer_info.request_type =
        chrome_usb::ControlTransferInfoRequestType::kStandard;
    transfer_info.request = kTransferRequestField;
    transfer_info.value = kTransferValueField;
    transfer_info.index = transfer_index;
    std::vector<uint8_t> data = GenerateTransferData(transfer_index, is_output);
    if (is_output)
      transfer_info.data = std::move(data);
    else
      transfer_info.length = data.size();
    transfer_info.timeout = kTransferTimeout;
    return transfer_info;
  }

  static std::vector<uint8_t> GenerateTransferData(size_t transfer_index,
                                                   bool is_output) {
    std::vector<uint8_t> result;
    result.push_back(is_output);
    while (transfer_index) {
      result.push_back(transfer_index & 255);
      transfer_index >>= 8;
    }
    return result;
  }

  RequestResult<chrome_usb::TransferResult> GenerateTransferRequestResult(
      size_t transfer_index,
      bool is_output) {
    if (IsTransferToFail(transfer_index)) {
      return RequestResult<chrome_usb::TransferResult>::CreateFailed(
          "fake failure");
    }

    chrome_usb::TransferResult result;
    if (!IsTransferToFinishUnsuccessfully(transfer_index)) {
      result.result_info.result_code =
          chrome_usb::kTransferResultInfoSuccessResultCode;
      if (!is_output)
        result.result_info.data = GenerateTransferData(transfer_index, false);
    }
    return RequestResult<chrome_usb::TransferResult>::CreateSuccessful(result);
  }

  void OnAsyncControlTransferFinished(
      libusb_transfer* transfer,
      size_t transfer_index,
      bool is_output,
      MockFunction<void(libusb_transfer_status)>* transfer_callback) {
    if (transfer->status != LIBUSB_TRANSFER_CANCELLED) {
      EXPECT_EQ(GetExpectedTransferStatus(transfer_index, false),
                transfer->status);
    }

    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
      const std::vector<uint8_t> expected_data =
          GenerateTransferData(transfer_index, is_output);
      EXPECT_EQ(static_cast<int>(expected_data.size()),
                transfer->actual_length);
      if (!is_output) {
        const uint8_t* data = transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL
                                  ? libusb_control_transfer_get_data(transfer)
                                  : transfer->buffer;
        EXPECT_EQ(expected_data,
                  std::vector<uint8_t>(data, data + transfer->actual_length));
      }
    }

    transfer_callback->Call(transfer->status);
  }

  static libusb_transfer_status GetExpectedTransferStatus(size_t transfer_index,
                                                          bool is_canceled) {
    if (is_canceled)
      return LIBUSB_TRANSFER_CANCELLED;
    if (IsTransferToFail(transfer_index))
      return LIBUSB_TRANSFER_ERROR;
    if (IsTransferToFinishUnsuccessfully(transfer_index))
      return LIBUSB_TRANSFER_ERROR;
    return LIBUSB_TRANSFER_COMPLETED;
  }
};

struct LibusbJsProxySingleTransferTestParam {
  LibusbJsProxySingleTransferTestParam(size_t transfer_index,
                                       bool is_transfer_output)
      : transfer_index(transfer_index),
        is_transfer_output(is_transfer_output) {}

  size_t transfer_index;
  bool is_transfer_output;
};

class LibusbJsProxySingleTransferTest
    : public LibusbJsProxyTransfersTest,
      public ::testing::WithParamInterface<
          LibusbJsProxySingleTransferTestParam> {
 public:
  static size_t GetTransferIndexToSucceed() {
    const size_t kTransferIndex = 1234;
    GOOGLE_SMART_CARD_CHECK(!IsTransferToFail(kTransferIndex));
    GOOGLE_SMART_CARD_CHECK(!IsTransferToFinishUnsuccessfully(kTransferIndex));
    return kTransferIndex;
  }

  static size_t GetTransferIndexToFail() {
    const size_t kTransferIndex = 1230;
    GOOGLE_SMART_CARD_CHECK(IsTransferToFail(kTransferIndex));
    GOOGLE_SMART_CARD_CHECK(!IsTransferToFinishUnsuccessfully(kTransferIndex));
    return kTransferIndex;
  }

  static size_t GetTransferIndexToFinishUnsuccessful() {
    const size_t kTransferIndex = 1235;
    GOOGLE_SMART_CARD_CHECK(!IsTransferToFail(kTransferIndex));
    GOOGLE_SMART_CARD_CHECK(IsTransferToFinishUnsuccessfully(kTransferIndex));
    return kTransferIndex;
  }
};

}  // namespace

// Test a simple asynchronous control transfer.
//
// The transfer request is resolved on the same thread that initiated the
// transfer, before the libusb events handling starts.
TEST_P(LibusbJsProxySingleTransferTest, AsyncControlTransfer) {
  const std::function<void()> chrome_usb_transfer_resolver =
      SetUpMockForAsyncControlTransfer(GetParam().transfer_index,
                                       GetParam().is_transfer_output);

  MockFunction<void(libusb_transfer_status)> transfer_callback;
  StartAsyncControlTransfer(GetParam().transfer_index,
                            GetParam().is_transfer_output, &transfer_callback);

  chrome_usb_transfer_resolver();

  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&transfer_callback));
  SetUpTransferCallbackMockExpectations(GetParam().transfer_index, false,
                                        &transfer_callback);
  libusb_js_proxy->LibusbHandleEvents(nullptr);
}

// Test cancellation of an asynchronous control transfers.
//
// Note that output control transfers cancellation never succeeds.
TEST_P(LibusbJsProxySingleTransferTest, AsyncTransferCancellation) {
  const std::function<void()> chrome_usb_transfer_resolver =
      SetUpMockForAsyncControlTransfer(GetParam().transfer_index,
                                       GetParam().is_transfer_output);

  MockFunction<void(libusb_transfer_status)> transfer_callback;
  libusb_transfer* const transfer = StartAsyncControlTransfer(
      GetParam().transfer_index, GetParam().is_transfer_output,
      &transfer_callback);

  // Output transfer cancellation is never successfull
  const bool is_cancellation_successful = !GetParam().is_transfer_output;

  if (is_cancellation_successful) {
    EXPECT_EQ(LIBUSB_SUCCESS, libusb_js_proxy->LibusbCancelTransfer(transfer));
  } else {
    EXPECT_NE(LIBUSB_SUCCESS, libusb_js_proxy->LibusbCancelTransfer(transfer));
  }

  // Second attempt to cancel a transfer is never successful
  EXPECT_NE(LIBUSB_SUCCESS, libusb_js_proxy->LibusbCancelTransfer(transfer));

  if (!is_cancellation_successful) {
    // Resolve the chrome.usb transfer if the transfer was an output transfer,
    // because its cancellation was unsuccessful.
    chrome_usb_transfer_resolver();
  }

  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&transfer_callback));
  SetUpTransferCallbackMockExpectations(GetParam().transfer_index,
                                        is_cancellation_successful,
                                        &transfer_callback);
  libusb_js_proxy->LibusbHandleEvents(nullptr);
}

// Test that received result of a canceled asynchronous transfer is delivered to
// the next transfer with the same parameters.
TEST_P(LibusbJsProxySingleTransferTest,
       AsyncTransferCompletionAfterCancellation) {
  if (GetParam().is_transfer_output) {
    // Cancellation of an output transfer is not supported, so the whole test
    // makes no sense.
    return;
  }

  const std::function<void()> first_chrome_usb_transfer_resolver =
      SetUpMockForAsyncControlTransfer(GetParam().transfer_index,
                                       GetParam().is_transfer_output);
  const std::function<void()> second_chrome_usb_transfer_resolver =
      SetUpMockForAsyncControlTransfer(GetParam().transfer_index,
                                       GetParam().is_transfer_output);

  MockFunction<void(libusb_transfer_status)> first_transfer_callback;
  libusb_transfer* const first_transfer = StartAsyncControlTransfer(
      GetParam().transfer_index, GetParam().is_transfer_output,
      &first_transfer_callback);

  MockFunction<void(libusb_transfer_status)> second_transfer_callback;
  StartAsyncControlTransfer(GetParam().transfer_index,
                            GetParam().is_transfer_output,
                            &second_transfer_callback);

  EXPECT_EQ(LIBUSB_SUCCESS,
            libusb_js_proxy->LibusbCancelTransfer(first_transfer));

  first_chrome_usb_transfer_resolver();

  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&first_transfer_callback));
  ASSERT_TRUE(Mock::VerifyAndClearExpectations(&second_transfer_callback));
  SetUpTransferCallbackMockExpectations(GetParam().transfer_index, true,
                                        &first_transfer_callback);
  SetUpTransferCallbackMockExpectations(GetParam().transfer_index, false,
                                        &second_transfer_callback);
  libusb_js_proxy->LibusbHandleEvents(nullptr);
  libusb_js_proxy->LibusbHandleEvents(nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    InputTransferTest,
    LibusbJsProxySingleTransferTest,
    ::testing::Values(
        LibusbJsProxySingleTransferTestParam(
            LibusbJsProxySingleTransferTest::GetTransferIndexToSucceed(),
            false),
        LibusbJsProxySingleTransferTestParam(
            LibusbJsProxySingleTransferTest::GetTransferIndexToFail(),
            false),
        LibusbJsProxySingleTransferTestParam(
            LibusbJsProxySingleTransferTest::
                GetTransferIndexToFinishUnsuccessful(),
            false)));

INSTANTIATE_TEST_SUITE_P(
    OutputTransferTest,
    LibusbJsProxySingleTransferTest,
    ::testing::Values(
        LibusbJsProxySingleTransferTestParam(
            LibusbJsProxySingleTransferTest::GetTransferIndexToSucceed(),
            true),
        LibusbJsProxySingleTransferTestParam(
            LibusbJsProxySingleTransferTest::GetTransferIndexToFail(),
            true),
        LibusbJsProxySingleTransferTestParam(
            LibusbJsProxySingleTransferTest::
                GetTransferIndexToFinishUnsuccessful(),
            true)));

namespace {

class LibusbJsProxyAsyncTransfersMultiThreadingTest
    : public LibusbJsProxyTransfersTest {
 public:
  void SetUp() override {
    LibusbJsProxyTransfersTest::SetUp();

    for (size_t index = 0; index <= kMaxTransferIndex; ++index) {
      for (bool is_transfer_output : kBoolValues) {
        chrome_usb_transfer_resolvers_[{index, is_transfer_output}] =
            SetUpMockForAsyncControlTransfer(index, is_transfer_output);
      }
    }
  }

  void TearDown() override {
    chrome_usb_transfer_resolvers_.clear();
    transfers_in_flight_.clear();

    LibusbJsProxyTransfersTest::TearDown();
  }

 protected:
  // A high number of transfers increases the chances of catching a bug, but the
  // constant is lower in the Debug mode to avoid running too long.
  const size_t kMaxTransferIndex =
#ifdef NDEBUG
      1000
#else
      100
#endif
      ;

  void AddTransferInFlight(size_t transfer_index, bool is_transfer_output) {
    const std::unique_lock<std::mutex> lock(mutex_);
    transfers_in_flight_.emplace_back(transfer_index, is_transfer_output);
    condition_.notify_all();
  }

  bool WaitAndGetTransferInFlight(
      size_t* transfer_index,
      bool* is_transfer_output,
      std::function<void()>* chrome_usb_transfer_resolver) {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;) {
      if (!transfers_in_flight_.empty()) {
        *transfer_index = transfers_in_flight_.back().first;
        *is_transfer_output = transfers_in_flight_.back().second;

        *chrome_usb_transfer_resolver =
            chrome_usb_transfer_resolvers_[transfers_in_flight_.back()];
        chrome_usb_transfer_resolvers_.erase(transfers_in_flight_.back());

        transfers_in_flight_.pop_back();
        return true;
      }

      if (chrome_usb_transfer_resolvers_.empty())
        return false;

      condition_.wait(lock);
    }
  }

 private:
  using TransferIndexAndIsOutput = std::pair<size_t, bool>;

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::map<TransferIndexAndIsOutput, std::function<void()>>
      chrome_usb_transfer_resolvers_;
  std::vector<TransferIndexAndIsOutput> transfers_in_flight_;
};

}  // namespace

// Test the correctness of work of multiple threads issuing a sequence of
// asynchronous transfer requests and running libusb event loops.
//
// The requests are resolved asynchronously from the main thread.
TEST_F(LibusbJsProxyAsyncTransfersMultiThreadingTest, ControlTransfers) {
  const size_t kThreadCount = 10;

  std::vector<std::thread> threads;
  for (size_t thread_index = 0; thread_index < kThreadCount; ++thread_index) {
    threads.emplace_back([this, thread_index] {
      for (size_t transfer_index = thread_index;
           transfer_index <= kMaxTransferIndex;
           transfer_index += kThreadCount) {
        MockFunction<void(libusb_transfer_status)> transfer_callback[2];
        int transfer_completed[2] = {0};

        for (bool is_transfer_output : kBoolValues) {
          SetUpTransferCallbackMockExpectations(
              transfer_index, false, &transfer_callback[is_transfer_output],
              &transfer_completed[is_transfer_output]);
          StartAsyncControlTransfer(transfer_index, is_transfer_output,
                                    &transfer_callback[is_transfer_output]);
          AddTransferInFlight(transfer_index, is_transfer_output);
        }

        for (bool is_transfer_output : kBoolValues) {
          while (!transfer_completed[is_transfer_output]) {
            libusb_js_proxy->LibusbHandleEventsCompleted(
                nullptr, &transfer_completed[is_transfer_output]);
          }
        }
      }
    });
  }

  for (;;) {
    size_t transfer_index;
    bool is_transfer_output;
    std::function<void()> chrome_usb_transfer_resolver;
    if (!WaitAndGetTransferInFlight(&transfer_index, &is_transfer_output,
                                    &chrome_usb_transfer_resolver)) {
      break;
    }
    chrome_usb_transfer_resolver();
  }

  for (size_t thread_index = 0; thread_index < kThreadCount; ++thread_index)
    threads[thread_index].join();
}

// TODO(emaxx): Add tests for bulk and interrupt transfers

#endif

}  // namespace google_smart_card
