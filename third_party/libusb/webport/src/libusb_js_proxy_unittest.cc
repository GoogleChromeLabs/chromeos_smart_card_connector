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

#include "third_party/libusb/webport/src/libusb_js_proxy.h"

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

#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/value.h"

#include "common/cpp/src/public/testing_global_context.h"
#include "common/cpp/src/public/value_builder.h"
#include "third_party/libusb/webport/src/libusb_tracing_wrapper.h"

using testing::_;
using testing::Assign;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::MockFunction;
using testing::SizeIs;
using testing::WithArgs;

namespace google_smart_card {

namespace {

constexpr int kControlTransferRequest = 1;
constexpr int kControlTransferIndex = 24;
constexpr int kControlTransferValue = 42;

// Used with parameterized tests: whether or not a test should additionally
// use `LibusbTracingWrapper`.
enum class WrapperTestParam {
  kWithTracingWrapper,
  kWithoutTracingWrapper,
};

std::string PrintWrapperTestParam(
    const testing::TestParamInfo<WrapperTestParam>& info) {
  switch (info.param) {
    case WrapperTestParam::kWithTracingWrapper:
      return "WithTracingWrapper";
    case WrapperTestParam::kWithoutTracingWrapper:
      return "WithoutTracingWrapper";
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

// Prepares a setup packet for an input control transfer.
std::vector<uint8_t> MakeLibusbInputControlTransferSetup(
    int data_length_requested) {
  std::vector<uint8_t> setup(LIBUSB_CONTROL_SETUP_SIZE + data_length_requested);
  libusb_fill_control_setup(setup.data(),
                            LIBUSB_RECIPIENT_ENDPOINT |
                                LIBUSB_REQUEST_TYPE_STANDARD |
                                LIBUSB_ENDPOINT_IN,
                            kControlTransferRequest, kControlTransferValue,
                            kControlTransferIndex, data_length_requested);
  return setup;
}

// Prepares a setup packet for an output control transfer.
std::vector<uint8_t> MakeLibusbOutputControlTransferSetup(
    const std::vector<uint8_t>& data_to_send) {
  std::vector<uint8_t> setup(LIBUSB_CONTROL_SETUP_SIZE + data_to_send.size());
  libusb_fill_control_setup(setup.data(),
                            LIBUSB_RECIPIENT_ENDPOINT |
                                LIBUSB_REQUEST_TYPE_STANDARD |
                                LIBUSB_ENDPOINT_OUT,
                            kControlTransferRequest, kControlTransferValue,
                            kControlTransferIndex, data_to_send.size());
  std::copy(data_to_send.begin(), data_to_send.end(),
            setup.begin() + LIBUSB_CONTROL_SETUP_SIZE);
  return setup;
}

// The default callback used for `libusb_transfer`. It signals to the test that
// the transfer is completed.
void OnLibusbAsyncTransferCompleted(libusb_transfer* transfer) {
  ASSERT_TRUE(transfer);
  // `user_data` points to `transfer_completion_flag` (a captureless function
  // pointer has no other way of telling the test it's run).
  *static_cast<int*>(transfer->user_data) = 1;
}

}  // namespace

// Fixture for testing `LibusbJsProxy` with/without `LibusbTracingWrapper`.
class LibusbJsProxyTest : public ::testing::TestWithParam<WrapperTestParam> {
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

  TestingGlobalContext* global_context() { return &global_context_; }
  LibusbInterface* libusb() {
    return GetParam() == WrapperTestParam::kWithTracingWrapper
               ? static_cast<LibusbInterface*>(&libusb_tracing_wrapper_)
               : static_cast<LibusbInterface*>(&libusb_js_proxy_);
  }

  // A convenience wrapper around `LibusbJsProxy::LibusbGetDeviceList()`.
  std::vector<libusb_device*> GetLibusbDevices() {
    libusb_device** device_list = nullptr;
    ssize_t ret_code =
        libusb()->LibusbGetDeviceList(/*ctx=*/nullptr, &device_list);
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
    libusb()->LibusbFreeDeviceList(device_list, /*unref_devices=*/false);
    return devices;
  }

  void FreeLibusbDevices(const std::vector<libusb_device*>& devices) {
    for (auto* device : devices)
      libusb()->LibusbUnrefDevice(device);
  }

  // Submits the transfer and waits until the it completes using
  // `LibusbHandleEventsCompleted()`.
  void SubmitLibusbTransferAndWaitForCompletion(libusb_transfer& transfer,
                                                int& transfer_completed) {
    EXPECT_EQ(libusb()->LibusbSubmitTransfer(&transfer), LIBUSB_SUCCESS);
    EXPECT_FALSE(transfer_completed);
    WaitForLibusbTransferCompletion(transfer_completed);
  }

  // Waits until the transfer completes using `LibusbHandleEventsCompleted()`.
  void WaitForLibusbTransferCompletion(int& transfer_completed) {
    do {
      EXPECT_EQ(libusb()->LibusbHandleEventsCompleted(/*ctx=*/nullptr,
                                                      &transfer_completed),
                LIBUSB_SUCCESS);
    } while (!transfer_completed);
  }

  // Waits until the transfer completes using `LibusbHandleEvents()`. It's an
  // older Libusb API, with `LibusbHandleEventsCompleted()` recommended instead.
  void WaitForLibusbTransferCompletionViaOldApi(int& transfer_completed) {
    do {
      EXPECT_EQ(libusb()->LibusbHandleEvents(/*ctx=*/nullptr), LIBUSB_SUCCESS);
    } while (!transfer_completed);
  }

 private:
  TypedMessageRouter typed_message_router_;
  TestingGlobalContext global_context_{&typed_message_router_};
  LibusbJsProxy libusb_js_proxy_{&global_context_, &typed_message_router_};
  LibusbTracingWrapper libusb_tracing_wrapper_{&libusb_js_proxy_};
};

// Test `LibusbInit()` and `LibusbExit()`.
TEST_P(LibusbJsProxyTest, ContextsCreation) {
  EXPECT_EQ(libusb()->LibusbInit(nullptr), LIBUSB_SUCCESS);

  // Initializing a default context for the second time doesn't do anything
  EXPECT_EQ(libusb()->LibusbInit(nullptr), LIBUSB_SUCCESS);

  libusb_context* context_1 = nullptr;
  EXPECT_EQ(libusb()->LibusbInit(&context_1), LIBUSB_SUCCESS);
  EXPECT_TRUE(context_1);

  libusb_context* context_2 = nullptr;
  EXPECT_EQ(libusb()->LibusbInit(&context_2), LIBUSB_SUCCESS);
  EXPECT_TRUE(context_2);
  EXPECT_NE(context_1, context_2);

  libusb()->LibusbExit(context_1);
  libusb()->LibusbExit(context_2);
  libusb()->LibusbExit(/*ctx=*/nullptr);
}

// Test `LibusbGetDeviceList()` failure when the JS side returns an error.
TEST_P(LibusbJsProxyTest, DevicesListingWithFailure) {
  // Arrange:
  global_context()->WillReplyToRequestWithError(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray), "fake failure");

  // Act:
  libusb_device** device_list;
  EXPECT_EQ(libusb()->LibusbGetDeviceList(/*ctx=*/nullptr, &device_list),
            LIBUSB_ERROR_OTHER);
}

// Test `LibusbGetDeviceList()` successful scenario with zero readers.
TEST_P(LibusbJsProxyTest, DevicesListingWithNoItems) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "listDevices", /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/Value(Value::Type::kArray));

  // Act.
  EXPECT_TRUE(GetLibusbDevices().empty());
}

// Test `LibusbGetDeviceList()` successful scenario with 2 readers.
TEST_P(LibusbJsProxyTest, DevicesListingWithTwoItems) {
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
  global_context()->WillReplyToRequestWith(
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
TEST_P(LibusbJsProxyTest, DevicesListFreeingWithNoItems) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "listDevices", /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/Value(Value::Type::kArray));

  // Act.
  libusb_device** device_list = nullptr;
  EXPECT_EQ(libusb()->LibusbGetDeviceList(/*ctx=*/nullptr, &device_list), 0);
  // The test can't really assert the readers were actually deallocated, but
  // running it under ASan should guarantee catching mistakes.
  libusb()->LibusbFreeDeviceList(device_list, /*unref_devices=*/true);
}

// Test `LibusbFreeDeviceList()` correctly cleans up a list with 2 readers when
// called with `unref_devices`=true.
TEST_P(LibusbJsProxyTest, DevicesListFreeingWithTwoItems) {
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
  global_context()->WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/std::move(fake_js_reply));

  // Act.
  libusb_device** device_list = nullptr;
  EXPECT_EQ(libusb()->LibusbGetDeviceList(/*ctx=*/nullptr, &device_list), 2);
  // The test can't really assert the readers were actually deallocated, but
  // running it under ASan should guarantee catching mistakes.
  libusb()->LibusbFreeDeviceList(device_list, /*unref_devices=*/true);
}

// Test `LibusbOpen()` successful scenario.
TEST_P(LibusbJsProxyTest, DeviceOpening) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
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
  global_context()->WillReplyToRequestWith(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*result_to_reply_with=*/Value(456));
  global_context()->WillReplyToRequestWith(
      "libusb", "closeDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Add(456).Get(),
      /*result_to_reply_with=*/Value());

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);
  // Connect to the device.
  libusb_device_handle* device_handle = nullptr;
  ASSERT_EQ(libusb()->LibusbOpen(devices[0], &device_handle), LIBUSB_SUCCESS);
  ASSERT_TRUE(device_handle);
  // Disconnect from the device.
  libusb()->LibusbClose(device_handle);

  FreeLibusbDevices(devices);
}

// Test `LibusbOpen()` failure when the JS side returns an error.
TEST_P(LibusbJsProxyTest, DeviceOpeningFailure) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
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
  global_context()->WillReplyToRequestWithError(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*error_to_reply_with=*/"fake error");

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);
  // Connect to the device.
  libusb_device_handle* device_handle = nullptr;
  EXPECT_EQ(libusb()->LibusbOpen(devices[0], &device_handle),
            LIBUSB_ERROR_OTHER);

  FreeLibusbDevices(devices);
}

// Test `LibusbClose()` doesn't crash when the JavaScript side reports an error.
TEST_P(LibusbJsProxyTest, DeviceClosingFailure) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
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
  global_context()->WillReplyToRequestWith(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*result_to_reply_with=*/Value(456));
  global_context()->WillReplyToRequestWithError(
      "libusb", "closeDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Add(456).Get(),
      /*error_to_reply_with=*/"fake error");

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);
  // Connect to the device.
  libusb_device_handle* device_handle = nullptr;
  ASSERT_EQ(libusb()->LibusbOpen(devices[0], &device_handle), LIBUSB_SUCCESS);
  ASSERT_TRUE(device_handle);
  // Disconnect from the device. The `LibusbClose()` function is void, and we
  // expect it to not crash despite the error simulated on the JS side.
  libusb()->LibusbClose(device_handle);

  FreeLibusbDevices(devices);
}

// Test the `LibusbOpenDeviceWithVidPid()` successful scenario.
TEST_P(LibusbJsProxyTest, DeviceOpeningWithVidPid) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray),
      /*result_to_reply_with=*/
      ArrayValueBuilder()
          .Add(DictValueBuilder()
                   .Add("deviceId", 123)
                   .Add("vendorId", 1)
                   .Add("productId", 2)
                   .Get())
          .Add(DictValueBuilder()
                   .Add("deviceId", 124)
                   .Add("vendorId", 3)
                   .Add("productId", 4)
                   .Get())
          .Get());
  global_context()->WillReplyToRequestWith(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(124).Get(),
      /*result_to_reply_with=*/Value(567));
  global_context()->WillReplyToRequestWith(
      "libusb", "closeDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(124).Add(567).Get(),
      /*result_to_reply_with=*/Value());

  // Act. Connect to one of the devices.
  libusb_device_handle* device_handle = libusb()->LibusbOpenDeviceWithVidPid(
      /*ctx=*/nullptr, /*vendor_id=*/3, /*product_id=*/4);
  ASSERT_TRUE(device_handle);
  // Disconnect from the device.
  libusb()->LibusbClose(device_handle);
}

// Test the `LibusbOpenDeviceWithVidPid()` failure scenario when the requested
// device doesn't exist.
TEST_P(LibusbJsProxyTest, DeviceOpeningWithVidPidFailureNonExisting) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
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

  // Act. Attempt to connect.
  libusb_device_handle* device_handle = libusb()->LibusbOpenDeviceWithVidPid(
      /*ctx=*/nullptr, /*vendor_id=*/3, /*product_id=*/4);
  EXPECT_FALSE(device_handle);
}

// Test the `LibusbOpenDeviceWithVidPid()` failure scenario caused by an error
// while enumerating the devices.
TEST_P(LibusbJsProxyTest, DeviceOpeningWithVidPidFailureToList) {
  // Arrange.
  global_context()->WillReplyToRequestWithError(
      "libusb", "listDevices",
      /*arguments=*/Value(Value::Type::kArray), "fake failure");

  // Act. Attempt to connect.
  libusb_device_handle* device_handle = libusb()->LibusbOpenDeviceWithVidPid(
      /*ctx=*/nullptr, /*vendor_id=*/1, /*product_id=*/2);
  EXPECT_FALSE(device_handle);
}

// Test the `LibusbOpenDeviceWithVidPid()` failure scenario caused by an error
// while opening the device.
TEST_P(LibusbJsProxyTest, DeviceOpeningWithVidPidFailureToOpen) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
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
  global_context()->WillReplyToRequestWithError(
      "libusb", "openDeviceHandle",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(), "fake failure");

  // Act. Attempt to connect.
  libusb_device_handle* device_handle = libusb()->LibusbOpenDeviceWithVidPid(
      /*ctx=*/nullptr, /*vendor_id=*/1, /*product_id=*/2);
  EXPECT_FALSE(device_handle);
}

// Test `LibusbRefDevice()` and `LibusbUnrefDevice()` that increment and
// decrement the libusb_device reference counter.
TEST_P(LibusbJsProxyTest, DeviceRefUnref) {
  constexpr int kIterationCount = 100;

  // Arrange.
  global_context()->WillReplyToRequestWith(
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
  EXPECT_EQ(libusb()->LibusbRefDevice(devices[0]), devices[0]);
  libusb()->LibusbUnrefDevice(devices[0]);
  // Increase and then decrease the reference counter by `kIterationCount`. Same
  // as above, we can't assert anything explicitly here.
  for (int i = 0; i < kIterationCount; ++i)
    EXPECT_EQ(libusb()->LibusbRefDevice(devices[0]), devices[0]);
  for (int i = 0; i < kIterationCount; ++i)
    libusb()->LibusbUnrefDevice(devices[0]);

  FreeLibusbDevices(devices);
}

// Test the behavior of `LibusbGetActiveConfigDescriptor()` on the parameters
// taken from the real SCM SCR 3310 device.
TEST_P(LibusbJsProxyTest, LibusbGetActiveConfigDescriptorScmScr3310) {
  const std::vector<uint8_t> kInterfaceExtraData{
      0x36, 0x21, 0x00, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0xa0,
      0x0f, 0x00, 0x00, 0xe0, 0x2e, 0x00, 0x00, 0x00, 0x80, 0x25, 0x00,
      0x00, 0x00, 0xb0, 0x04, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xba, 0x00, 0x01, 0x00,
      0x07, 0x01, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01};

  // Arrange.
  global_context()->WillReplyToRequestWith(
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
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_THAT(devices, SizeIs(1));
  global_context()->WillReplyToRequestWith(
      "libusb", "getConfigurations",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*result_to_reply_with=*/
      ArrayValueBuilder()
          .Add(
              DictValueBuilder()
                  .Add("active", true)
                  .Add("configurationValue", 1)
                  .Add(
                      "interfaces",
                      ArrayValueBuilder()
                          .Add(
                              DictValueBuilder()
                                  .Add("interfaceNumber", 0)
                                  .Add("interfaceClass", 11)
                                  .Add("interfaceSubclass", 0)
                                  .Add("interfaceProtocol", 0)
                                  .Add("extraData", kInterfaceExtraData)
                                  .Add("endpoints",
                                       ArrayValueBuilder()
                                           .Add(DictValueBuilder()
                                                    .Add("endpointAddress", 1)
                                                    .Add("direction", "out")
                                                    .Add("type", "bulk")
                                                    .Add("maxPacketSize", 64)
                                                    .Get())
                                           .Add(DictValueBuilder()
                                                    .Add("endpointAddress", 130)
                                                    .Add("direction", "in")
                                                    .Add("type", "bulk")
                                                    .Add("maxPacketSize", 64)
                                                    .Get())
                                           .Add(DictValueBuilder()
                                                    .Add("endpointAddress", 131)
                                                    .Add("direction", "in")
                                                    .Add("type", "interrupt")
                                                    .Add("maxPacketSize", 16)
                                                    .Get())
                                           .Get())
                                  .Get())
                          .Get())
                  .Get())
          .Get());

  // Act.
  libusb_config_descriptor* descriptor = nullptr;
  EXPECT_EQ(libusb()->LibusbGetActiveConfigDescriptor(devices[0], &descriptor),
            LIBUSB_SUCCESS);

  // Assert.
  ASSERT_TRUE(descriptor);
  EXPECT_EQ(descriptor->bLength, sizeof(libusb_config_descriptor));
  EXPECT_EQ(descriptor->bDescriptorType, LIBUSB_DT_CONFIG);
  EXPECT_EQ(descriptor->wTotalLength, sizeof(libusb_config_descriptor));
  EXPECT_EQ(descriptor->bConfigurationValue, 1);
  EXPECT_EQ(descriptor->bNumInterfaces, 1);
  EXPECT_EQ(descriptor->extra, nullptr);
  EXPECT_EQ(descriptor->extra_length, 0);
  const auto& interface = descriptor->interface[0];
  EXPECT_EQ(interface.num_altsetting, 1);
  const auto& interface_descriptor = interface.altsetting[0];
  EXPECT_EQ(interface_descriptor.bLength, sizeof(libusb_interface_descriptor));
  EXPECT_EQ(interface_descriptor.bDescriptorType, LIBUSB_DT_INTERFACE);
  EXPECT_EQ(interface_descriptor.bInterfaceNumber, 0);
  EXPECT_EQ(interface_descriptor.bInterfaceClass, 11);
  EXPECT_EQ(interface_descriptor.bInterfaceSubClass, 0);
  EXPECT_EQ(interface_descriptor.bInterfaceProtocol, 0);
  ASSERT_TRUE(interface_descriptor.extra);
  ASSERT_GT(interface_descriptor.extra_length, 0);
  EXPECT_EQ(std::vector<uint8_t>(
                interface_descriptor.extra,
                interface_descriptor.extra + interface_descriptor.extra_length),
            kInterfaceExtraData);
  ASSERT_EQ(interface_descriptor.bNumEndpoints, 3);
  const auto* endpoint = interface_descriptor.endpoint;
  ASSERT_TRUE(endpoint);
  EXPECT_EQ(endpoint[0].bLength, sizeof(libusb_endpoint_descriptor));
  EXPECT_EQ(endpoint[0].bDescriptorType, LIBUSB_DT_ENDPOINT);
  EXPECT_EQ(endpoint[0].bEndpointAddress, 1);
  EXPECT_EQ(endpoint[0].bmAttributes, LIBUSB_TRANSFER_TYPE_BULK);
  EXPECT_EQ(endpoint[0].wMaxPacketSize, 64);
  EXPECT_EQ(endpoint[0].extra, nullptr);
  EXPECT_EQ(endpoint[0].extra_length, 0);
  EXPECT_EQ(endpoint[1].bLength, sizeof(libusb_endpoint_descriptor));
  EXPECT_EQ(endpoint[1].bDescriptorType, LIBUSB_DT_ENDPOINT);
  EXPECT_EQ(endpoint[1].bEndpointAddress, 130);
  EXPECT_EQ(endpoint[1].bmAttributes, LIBUSB_TRANSFER_TYPE_BULK);
  EXPECT_EQ(endpoint[1].wMaxPacketSize, 64);
  EXPECT_EQ(endpoint[1].extra, nullptr);
  EXPECT_EQ(endpoint[1].extra_length, 0);
  EXPECT_EQ(endpoint[2].bLength, sizeof(libusb_endpoint_descriptor));
  EXPECT_EQ(endpoint[2].bDescriptorType, LIBUSB_DT_ENDPOINT);
  EXPECT_EQ(endpoint[2].bEndpointAddress, 131);
  EXPECT_EQ(endpoint[2].bmAttributes, LIBUSB_TRANSFER_TYPE_INTERRUPT);
  EXPECT_EQ(endpoint[2].wMaxPacketSize, 16);
  EXPECT_EQ(endpoint[2].extra, nullptr);
  EXPECT_EQ(endpoint[2].extra_length, 0);

  // Cleanup.
  libusb()->LibusbFreeConfigDescriptor(descriptor);
  FreeLibusbDevices(devices);
}

// Test the behavior of `LibusbGetActiveConfigDescriptor()` on the parameters
// taken from the real Dell Smart Card Reader Keyboard. In this case some
// (non-smart-card) USB interfaces are skipped, hence the result contains
// sentinel records.
TEST_P(LibusbJsProxyTest, LibusbGetActiveConfigDescriptorDellKeyboard) {
  const std::vector<uint8_t> kInterfaceExtraData{
      0x36, 0x21, 0x01, 0x01, 0x00, 0x07, 0x03, 0x00, 0x00, 0x00, 0xc0,
      0x12, 0x00, 0x00, 0xc0, 0x12, 0x00, 0x00, 0x00, 0x67, 0x32, 0x00,
      0x00, 0xce, 0x99, 0x0c, 0x00, 0x35, 0xfe, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x01, 0x00,
      0x0f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01};

  // Arrange.
  global_context()->WillReplyToRequestWith(
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
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_THAT(devices, SizeIs(1));
  // Configure mock USB configuration. Note the "interfaceNumber" value that's
  // equal to "1", as it makes the test different from above by leaving out the
  // interface "0".
  global_context()->WillReplyToRequestWith(
      "libusb", "getConfigurations",
      /*arguments=*/ArrayValueBuilder().Add(123).Get(),
      /*result_to_reply_with=*/
      ArrayValueBuilder()
          .Add(
              DictValueBuilder()
                  .Add("active", true)
                  .Add("configurationValue", 1)
                  .Add(
                      "interfaces",
                      ArrayValueBuilder()
                          .Add(
                              DictValueBuilder()
                                  .Add("interfaceNumber", 1)
                                  .Add("interfaceClass", 11)
                                  .Add("interfaceSubclass", 0)
                                  .Add("interfaceProtocol", 0)
                                  .Add("extraData", kInterfaceExtraData)
                                  .Add("endpoints",
                                       ArrayValueBuilder()
                                           .Add(DictValueBuilder()
                                                    .Add("endpointAddress", 1)
                                                    .Add("direction", "out")
                                                    .Add("type", "bulk")
                                                    .Add("maxPacketSize", 64)
                                                    .Get())
                                           .Add(DictValueBuilder()
                                                    .Add("endpointAddress", 130)
                                                    .Add("direction", "in")
                                                    .Add("type", "bulk")
                                                    .Add("maxPacketSize", 64)
                                                    .Get())
                                           .Add(DictValueBuilder()
                                                    .Add("endpointAddress", 131)
                                                    .Add("direction", "in")
                                                    .Add("type", "interrupt")
                                                    .Add("maxPacketSize", 8)
                                                    .Get())
                                           .Get())
                                  .Get())
                          .Get())
                  .Get())
          .Get());

  // Act.
  libusb_config_descriptor* descriptor = nullptr;
  EXPECT_EQ(libusb()->LibusbGetActiveConfigDescriptor(devices[0], &descriptor),
            LIBUSB_SUCCESS);

  // Assert.
  ASSERT_TRUE(descriptor);
  EXPECT_EQ(descriptor->bLength, sizeof(libusb_config_descriptor));
  EXPECT_EQ(descriptor->bDescriptorType, LIBUSB_DT_CONFIG);
  EXPECT_EQ(descriptor->wTotalLength, sizeof(libusb_config_descriptor));
  EXPECT_EQ(descriptor->bConfigurationValue, 1);
  EXPECT_EQ(descriptor->extra, nullptr);
  EXPECT_EQ(descriptor->extra_length, 0);
  ASSERT_EQ(descriptor->bNumInterfaces, 2);
  const auto& interface0 = descriptor->interface[0];
  const auto& interface1 = descriptor->interface[1];
  EXPECT_EQ(interface1.num_altsetting, 0);
  EXPECT_EQ(interface1.altsetting, nullptr);
  EXPECT_EQ(interface0.num_altsetting, 1);
  const auto& interface_descriptor = interface0.altsetting[0];
  EXPECT_EQ(interface_descriptor.bLength, sizeof(libusb_interface_descriptor));
  EXPECT_EQ(interface_descriptor.bDescriptorType, LIBUSB_DT_INTERFACE);
  EXPECT_EQ(interface_descriptor.bInterfaceNumber, 1);
  EXPECT_EQ(interface_descriptor.bInterfaceClass, 11);
  EXPECT_EQ(interface_descriptor.bInterfaceSubClass, 0);
  EXPECT_EQ(interface_descriptor.bInterfaceProtocol, 0);
  ASSERT_TRUE(interface_descriptor.extra);
  ASSERT_GT(interface_descriptor.extra_length, 0);
  EXPECT_EQ(std::vector<uint8_t>(
                interface_descriptor.extra,
                interface_descriptor.extra + interface_descriptor.extra_length),
            kInterfaceExtraData);
  ASSERT_EQ(interface_descriptor.bNumEndpoints, 3);
  const auto* endpoint = interface_descriptor.endpoint;
  ASSERT_TRUE(endpoint);
  EXPECT_EQ(endpoint[0].bLength, sizeof(libusb_endpoint_descriptor));
  EXPECT_EQ(endpoint[0].bDescriptorType, LIBUSB_DT_ENDPOINT);
  EXPECT_EQ(endpoint[0].bEndpointAddress, 1);
  EXPECT_EQ(endpoint[0].bmAttributes, LIBUSB_TRANSFER_TYPE_BULK);
  EXPECT_EQ(endpoint[0].wMaxPacketSize, 64);
  EXPECT_EQ(endpoint[0].extra, nullptr);
  EXPECT_EQ(endpoint[0].extra_length, 0);
  EXPECT_EQ(endpoint[1].bLength, sizeof(libusb_endpoint_descriptor));
  EXPECT_EQ(endpoint[1].bDescriptorType, LIBUSB_DT_ENDPOINT);
  EXPECT_EQ(endpoint[1].bEndpointAddress, 130);
  EXPECT_EQ(endpoint[1].bmAttributes, LIBUSB_TRANSFER_TYPE_BULK);
  EXPECT_EQ(endpoint[1].wMaxPacketSize, 64);
  EXPECT_EQ(endpoint[1].extra, nullptr);
  EXPECT_EQ(endpoint[1].extra_length, 0);
  EXPECT_EQ(endpoint[2].bLength, sizeof(libusb_endpoint_descriptor));
  EXPECT_EQ(endpoint[2].bDescriptorType, LIBUSB_DT_ENDPOINT);
  EXPECT_EQ(endpoint[2].bEndpointAddress, 131);
  EXPECT_EQ(endpoint[2].bmAttributes, LIBUSB_TRANSFER_TYPE_INTERRUPT);
  EXPECT_EQ(endpoint[2].wMaxPacketSize, 8);
  EXPECT_EQ(endpoint[2].extra, nullptr);
  EXPECT_EQ(endpoint[2].extra_length, 0);

  // Cleanup.
  libusb()->LibusbFreeConfigDescriptor(descriptor);
  FreeLibusbDevices(devices);
}

// Test that `LibusbGetBusNumber()` initially returns the default bus number.
TEST_P(LibusbJsProxyTest, BusNumber) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
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

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);

  // Assert.
  EXPECT_EQ(libusb()->LibusbGetBusNumber(devices[0]), 1);

  FreeLibusbDevices(devices);
}

// Test that `LibusbGetBusNumber()` returns the same default bus number when
// the devices are listed for the second time.
TEST_P(LibusbJsProxyTest, BusNumberConstant) {
  // Arrange.
  for (int i = 0; i < 2; ++i) {
    global_context()->WillReplyToRequestWith(
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
  }

  // Act. Obtain the device from the device list.
  std::vector<libusb_device*> devices = GetLibusbDevices();
  EXPECT_EQ(devices.size(), 1U);
  FreeLibusbDevices(devices);
  // List devices again.
  devices = GetLibusbDevices();
  ASSERT_EQ(devices.size(), 1U);

  // Assert.
  EXPECT_EQ(libusb()->LibusbGetBusNumber(devices[0]), 1);

  FreeLibusbDevices(devices);
}

INSTANTIATE_TEST_SUITE_P(
    ,
    LibusbJsProxyTest,
    testing::Values(WrapperTestParam::kWithoutTracingWrapper,
                    WrapperTestParam::kWithTracingWrapper),
    PrintWrapperTestParam);

// Test fixture that simulates a single USB device present, and automatically
// connects to the device in SetUp.
class LibusbJsProxyWithDeviceTest : public LibusbJsProxyTest {
 protected:
  static constexpr int kJsDeviceId = 123;
  static constexpr int kJsDeviceHandle = 456;

  LibusbJsProxyWithDeviceTest() = default;
  ~LibusbJsProxyWithDeviceTest() = default;

  void SetUp() override {
    ASSERT_EQ(libusb()->LibusbInit(/*ctx=*/nullptr), LIBUSB_SUCCESS);

    // Obtain the libusb device.
    global_context()->WillReplyToRequestWith(
        "libusb", "listDevices",
        /*arguments=*/Value(Value::Type::kArray), MakeListDevicesFakeJsReply());
    std::vector<libusb_device*> devices = GetLibusbDevices();
    ASSERT_EQ(devices.size(), 1U);
    device_ = devices[0];

    // Connect to the device.
    global_context()->WillReplyToRequestWith(
        "libusb", "openDeviceHandle",
        /*arguments=*/ArrayValueBuilder().Add(kJsDeviceId).Get(),
        /*result_to_reply_with=*/Value(kJsDeviceHandle));
    EXPECT_EQ(libusb()->LibusbOpen(device_, &device_handle_), LIBUSB_SUCCESS);
    EXPECT_TRUE(device_handle_);
  }

  void TearDown() override {
    // Close the libusb device handle, which triggers a call to JS.
    global_context()->WillReplyToRequestWith(
        "libusb", "closeDeviceHandle",
        /*arguments=*/
        ArrayValueBuilder().Add(kJsDeviceId).Add(kJsDeviceHandle).Get(),
        /*result_to_reply_with=*/Value());
    libusb()->LibusbClose(device_handle_);
    device_handle_ = nullptr;

    // Deallocate the libusb device.
    libusb()->LibusbUnrefDevice(device_);
    device_ = nullptr;

    // Free the libusb global state.
    libusb()->LibusbExit(/*ctx=*/nullptr);
  }

  static Value MakeListDevicesFakeJsReply() {
    return ArrayValueBuilder()
        .Add(DictValueBuilder()
                 .Add("deviceId", kJsDeviceId)
                 .Add("vendorId", 2)
                 .Add("productId", 3)
                 .Get())
        .Get();
  }

  static Value MakeExpectedOutputControlTransferJsArgs(
      const std::string& recipient,
      const std::string& request_type,
      const std::vector<uint8_t>& data_to_send) {
    return ArrayValueBuilder()
        .Add(kJsDeviceId)
        .Add(kJsDeviceHandle)
        .Add(DictValueBuilder()
                 .Add("dataToSend", data_to_send)
                 .Add("index", kControlTransferIndex)
                 .Add("recipient", recipient)
                 .Add("request", kControlTransferRequest)
                 .Add("requestType", request_type)
                 .Add("value", kControlTransferValue)
                 .Get())
        .Get();
  }

  static Value MakeExpectedInputControlTransferJsArgs(
      const std::string& recipient,
      const std::string& request_type,
      int length_to_receive) {
    return ArrayValueBuilder()
        .Add(kJsDeviceId)
        .Add(kJsDeviceHandle)
        .Add(DictValueBuilder()
                 .Add("index", kControlTransferIndex)
                 .Add("recipient", recipient)
                 .Add("request", kControlTransferRequest)
                 .Add("requestType", request_type)
                 .Add("value", kControlTransferValue)
                 .Add("lengthToReceive", length_to_receive)
                 .Get())
        .Get();
  }

  static Value MakeInputTransferFakeJsReply(
      const std::vector<uint8_t>& received_data) {
    return DictValueBuilder().Add("receivedData", received_data).Get();
  }

  static Value MakeOutputTransferFakeJsReply() {
    return Value(Value::Type::kDictionary);
  }

  libusb_transfer* InitLibusbControlTransfer(int timeout,
                                             std::vector<uint8_t>& setup,
                                             int& transfer_completion_flag) {
    libusb_transfer* const transfer =
        libusb()->LibusbAllocTransfer(/*iso_packets=*/0);
    if (!transfer) {
      ADD_FAILURE() << "LibusbAllocTransfer failed";
      return nullptr;
    }
    libusb_fill_control_transfer(
        transfer, device_handle_, setup.data(), &OnLibusbAsyncTransferCompleted,
        /*user_data=*/static_cast<void*>(&transfer_completion_flag), timeout);
    return transfer;
  }

  libusb_device* device_ = nullptr;
  libusb_device_handle* device_handle_ = nullptr;
};

// Test `LibusbResetDevice()` successful scenario.
TEST_P(LibusbJsProxyWithDeviceTest, DeviceResetting) {
  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "resetDevice",
      /*arguments=*/
      ArrayValueBuilder().Add(kJsDeviceId).Add(kJsDeviceHandle).Get(),
      /*result_to_reply_with=*/Value());

  // Act.
  EXPECT_EQ(libusb()->LibusbResetDevice(device_handle_), LIBUSB_SUCCESS);
}

// Test `LibusbResetDevice()` failure due to the JS call returning an error.
TEST_P(LibusbJsProxyWithDeviceTest, DeviceResettingFailure) {
  // Arrange.
  global_context()->WillReplyToRequestWithError(
      "libusb", "resetDevice",
      /*arguments=*/
      ArrayValueBuilder().Add(kJsDeviceId).Add(kJsDeviceHandle).Get(),
      /*error_to_reply_with=*/"fake error");

  // Act.
  EXPECT_EQ(libusb()->LibusbResetDevice(device_handle_), LIBUSB_ERROR_OTHER);
}

// Test `LibusbGetDevice()`.
TEST_P(LibusbJsProxyWithDeviceTest, GetDevice) {
  EXPECT_EQ(libusb()->LibusbGetDevice(device_handle_), device_);
}

// Test `LibusbClaimInterface()` successful scenario.
TEST_P(LibusbJsProxyWithDeviceTest, InterfaceClaiming) {
  constexpr int kInterfaceNumber = 12;
  // Arrange.
  global_context()->WillReplyToRequestWith("libusb", "claimInterface",
                                           /*arguments=*/
                                           ArrayValueBuilder()
                                               .Add(kJsDeviceId)
                                               .Add(kJsDeviceHandle)
                                               .Add(kInterfaceNumber)
                                               .Get(),
                                           /*result_to_reply_with=*/Value());

  // Act.
  EXPECT_EQ(libusb()->LibusbClaimInterface(device_handle_, kInterfaceNumber),
            LIBUSB_SUCCESS);
}

// Test `LibusbClaimInterface()` failure due to the JS call returning an error.
TEST_P(LibusbJsProxyWithDeviceTest, InterfaceClaimingFailure) {
  constexpr int kInterfaceNumber = 12;
  // Arrange.
  global_context()->WillReplyToRequestWithError(
      "libusb", "claimInterface",
      /*arguments=*/
      ArrayValueBuilder()
          .Add(kJsDeviceId)
          .Add(kJsDeviceHandle)
          .Add(kInterfaceNumber)
          .Get(),
      /*error_to_reply_with=*/"fake error");

  // Act.
  EXPECT_EQ(libusb()->LibusbClaimInterface(device_handle_, kInterfaceNumber),
            LIBUSB_ERROR_OTHER);
}

// Tests `LibusbControlTransfer()` successful scenario when sending data to the
// output endpoint.
TEST_P(LibusbJsProxyWithDeviceTest, OutputControlTransfer) {
  // non-const, as LibusbControlTransfer() takes a non-const pointer to it -
  // following libusb's original interface.
  std::vector<uint8_t> data = {1, 2, 3};

  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "controlTransfer",
      MakeExpectedOutputControlTransferJsArgs(
          /*recipient=*/"endpoint", /*request_type=*/"standard", data),
      MakeOutputTransferFakeJsReply());

  // Act.
  EXPECT_EQ(libusb()->LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                    LIBUSB_ENDPOINT_OUT,
                kControlTransferRequest, kControlTransferValue,
                kControlTransferIndex, &data[0], data.size(), /*timeout=*/100),
            static_cast<int>(data.size()));
}

// Test `LibusbControlTransfer()` failure scenario due to a JS error during an
// output transfer.
TEST_P(LibusbJsProxyWithDeviceTest, OutputControlTransferFailure) {
  // non-const, as LibusbControlTransfer() takes a non-const pointer to it -
  // following libusb's original interface.
  std::vector<uint8_t> data = {1, 2, 3};

  // Arrange.
  global_context()->WillReplyToRequestWithError(
      "libusb", "controlTransfer",
      MakeExpectedOutputControlTransferJsArgs(/*recipient=*/"interface",
                                              /*request_type=*/"class", data),
      "fake error");

  // Act.
  EXPECT_EQ(libusb()->LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_INTERFACE | LIBUSB_REQUEST_TYPE_CLASS |
                    LIBUSB_ENDPOINT_OUT,
                kControlTransferRequest, kControlTransferValue,
                kControlTransferIndex, &data[0], data.size(), /*timeout=*/100),
            LIBUSB_ERROR_OTHER);
}

// Tests `LibusbControlTransfer()` successful scenario when reading data from
// an endpoint.
TEST_P(LibusbJsProxyWithDeviceTest, InputControlTransfer) {
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "controlTransfer",
      MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                             /*request_type=*/"standard",
                                             kData.size()),
      MakeInputTransferFakeJsReply(kData));

  // Act.
  std::vector<uint8_t> received_data(kData.size());
  EXPECT_EQ(
      libusb()->LibusbControlTransfer(
          device_handle_,
          LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
              LIBUSB_ENDPOINT_IN,
          kControlTransferRequest, kControlTransferValue, kControlTransferIndex,
          &received_data[0], received_data.size(), /*timeout=*/100),
      static_cast<int>(kData.size()));
  EXPECT_EQ(received_data, kData);
}

// Tests `LibusbControlTransfer()` scenario when the data read from an endpoint
// turned out to be shorter than requested.
TEST_P(LibusbJsProxyWithDeviceTest, InputControlTransferShorterData) {
  constexpr int kDataLengthRequested = 100;
  const std::vector<uint8_t> kDataResponded = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "controlTransfer",
      MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                             /*request_type=*/"standard",
                                             kDataLengthRequested),
      MakeInputTransferFakeJsReply(kDataResponded));

  // Act.
  std::vector<uint8_t> received_data(kDataLengthRequested);
  EXPECT_EQ(
      libusb()->LibusbControlTransfer(
          device_handle_,
          LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
              LIBUSB_ENDPOINT_IN,
          kControlTransferRequest, kControlTransferValue, kControlTransferIndex,
          &received_data[0], received_data.size(), /*timeout=*/100),
      static_cast<int>(kDataResponded.size()));
  EXPECT_EQ(std::vector<uint8_t>(received_data.begin(),
                                 received_data.begin() + kDataResponded.size()),
            kDataResponded);
}

// Tests `LibusbControlTransfer()` failure scenario when JS input transfer
// returned an error.
TEST_P(LibusbJsProxyWithDeviceTest, InputControlTransferFailure) {
  constexpr int kDataLengthRequested = 100;

  // Arrange.
  global_context()->WillReplyToRequestWithError(
      "libusb", "controlTransfer",
      MakeExpectedInputControlTransferJsArgs(/*recipient=*/"device",
                                             /*request_type=*/"vendor",
                                             kDataLengthRequested),
      /*error_to_reply_with=*/"fake error");

  // Act.
  std::vector<uint8_t> received_data(kDataLengthRequested);
  EXPECT_EQ(
      libusb()->LibusbControlTransfer(
          device_handle_,
          LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_VENDOR |
              LIBUSB_ENDPOINT_IN,
          kControlTransferRequest, kControlTransferValue, kControlTransferIndex,
          &received_data[0], received_data.size(), /*timeout=*/100),
      LIBUSB_ERROR_OTHER);
}

// Tests `LibusbControlTransfer()` timeout scenario for an input transfer.
TEST_P(LibusbJsProxyWithDeviceTest, InputControlTransferTimeout) {
  constexpr int kDataLengthRequested = 100;

  // Arrange. Set up the expectation for the request message. We won't reply to
  // this message.
  auto waiter = global_context()->CreateRequestWaiter(
      "libusb", "controlTransfer",
      MakeExpectedInputControlTransferJsArgs(/*recipient=*/"device",
                                             /*request_type=*/"vendor",
                                             kDataLengthRequested));

  // Act. This call will block for about a second before returning (we don't
  // verify the clocks to avoid flakiness).
  std::vector<uint8_t> received_data(kDataLengthRequested);
  EXPECT_EQ(
      libusb()->LibusbControlTransfer(
          device_handle_,
          LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_VENDOR |
              LIBUSB_ENDPOINT_IN,
          kControlTransferRequest, kControlTransferValue, kControlTransferIndex,
          &received_data[0], received_data.size(), /*timeout=*/1000),
      LIBUSB_ERROR_TIMEOUT);
}

// Tests `LibusbControlTransfer()` timeout scenario for an output transfer.
TEST_P(LibusbJsProxyWithDeviceTest, OutputControlTransferTimeout) {
  // non-const, as LibusbControlTransfer() takes a non-const pointer to it -
  // following libusb's original interface.
  std::vector<uint8_t> data = {1, 2, 3};

  // Arrange. Set up the expectation for the request message. We won't reply to
  // this message.
  auto waiter = global_context()->CreateRequestWaiter(
      "libusb", "controlTransfer",
      MakeExpectedOutputControlTransferJsArgs(
          /*recipient=*/"endpoint", /*request_type=*/"standard", data));

  // Act. This call will block for about a second before returning (we don't
  // verify the clocks to avoid flakiness).
  EXPECT_EQ(libusb()->LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                    LIBUSB_ENDPOINT_OUT,
                kControlTransferRequest, kControlTransferValue,
                kControlTransferIndex, &data[0], data.size(), /*timeout=*/1000),
            LIBUSB_ERROR_TIMEOUT);
}

// Test the correctness of work of multiple threads issuing a sequence of
// synchronous transfer requests. It's a regression test for #464 and #465.
//
// Each transfer request is resolved immediately on the same thread that
// initiated the transfer.
TEST_P(LibusbJsProxyWithDeviceTest, ControlTransfersMultiThreadedStressTest) {
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
  constexpr int kTotalIterations = kThreadCount * kIterationsPerThread;

  // Arrange.
  std::vector<std::unique_ptr<TestingGlobalContext::Waiter>>
      input_transfer_waiters, output_transfer_waiters;
  for (int i = 0; i < kTotalIterations; ++i) {
    // Each test thread iteration consists of one input and one output transfer
    // - start waiting for them in advance. We don't use
    // `WillReplyToRequestWith()`, because it'll lead to immediate reentrant
    // replies and deep recursion levels in the test (something that's not
    // possible in production, where USB requests are never resolved
    // synchronously).
    input_transfer_waiters.push_back(global_context()->CreateRequestWaiter(
        "libusb", "controlTransfer",
        MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                               /*request_type=*/"standard",
                                               kData.size())));
    output_transfer_waiters.push_back(global_context()->CreateRequestWaiter(
        "libusb", "controlTransfer",
        MakeExpectedOutputControlTransferJsArgs(
            /*recipient=*/"endpoint", /*request_type=*/"standard", kData)));
  }

  // Act.
  std::vector<std::thread> threads;
  for (int thread_index = 0; thread_index < kThreadCount; ++thread_index) {
    threads.emplace_back([this, kData] {
      for (int iteration = 0; iteration < kIterationsPerThread; ++iteration) {
        // Test input transfer.
        std::vector<uint8_t> received_data(kData.size());
        EXPECT_EQ(libusb()->LibusbControlTransfer(
                      device_handle_,
                      LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                          LIBUSB_ENDPOINT_IN,
                      kControlTransferRequest, kControlTransferValue,
                      kControlTransferIndex, &received_data[0],
                      received_data.size(), /*timeout=*/0),
                  static_cast<int>(kData.size()));
        EXPECT_EQ(received_data, kData);
        // Test output transfer.
        std::vector<uint8_t> data = kData;
        EXPECT_EQ(
            libusb()->LibusbControlTransfer(
                device_handle_,
                LIBUSB_RECIPIENT_ENDPOINT | LIBUSB_REQUEST_TYPE_STANDARD |
                    LIBUSB_ENDPOINT_OUT,
                kControlTransferRequest, kControlTransferValue,
                kControlTransferIndex, &data[0], data.size(), /*timeout=*/0),
            static_cast<int>(data.size()));
      }
    });
  }
  for (int i = 0; i < kTotalIterations; ++i) {
    input_transfer_waiters[i]->Wait();
    input_transfer_waiters[i]->Reply(MakeInputTransferFakeJsReply(kData));
    output_transfer_waiters[i]->Wait();
    output_transfer_waiters[i]->Reply(MakeOutputTransferFakeJsReply());
  }
  for (size_t thread_index = 0; thread_index < kThreadCount; ++thread_index)
    threads[thread_index].join();
}

// Test an asynchronous input control transfer successful scenario.
TEST_P(LibusbJsProxyWithDeviceTest, AsyncInputControlTransfer) {
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "controlTransfer",
      MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                             /*request_type=*/"standard",
                                             kData.size()),
      MakeInputTransferFakeJsReply(kData));

  // Act.
  std::vector<uint8_t> setup =
      MakeLibusbInputControlTransferSetup(kData.size());
  int transfer_completion_flag = 0;
  libusb_transfer* const transfer =
      InitLibusbControlTransfer(/*timeout=*/0, setup, transfer_completion_flag);
  ASSERT_TRUE(transfer);
  SubmitLibusbTransferAndWaitForCompletion(*transfer, transfer_completion_flag);

  // Assert.
  EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_COMPLETED);
  EXPECT_EQ(transfer->actual_length, static_cast<int>(kData.size()));
  EXPECT_EQ(std::vector<uint8_t>(setup.begin() + LIBUSB_CONTROL_SETUP_SIZE,
                                 setup.end()),
            kData);
  // Attempting to cancel a completed transfer fails.
  EXPECT_NE(libusb()->LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);

  // Cleanup:
  libusb()->LibusbFreeTransfer(transfer);
}

// Test an input control transfer when it fails on the JS side.
TEST_P(LibusbJsProxyWithDeviceTest, AsyncInputControlTransferFailure) {
  constexpr int kDataLengthRequested = 100;

  // Arrange.
  global_context()->WillReplyToRequestWithError(
      "libusb", "controlTransfer",
      MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                             /*request_type=*/"standard",
                                             kDataLengthRequested),
      "Fake failure");

  // Act.
  std::vector<uint8_t> setup =
      MakeLibusbInputControlTransferSetup(kDataLengthRequested);
  int transfer_completion_flag = 0;
  libusb_transfer* transfer =
      InitLibusbControlTransfer(/*timeout=*/0, setup, transfer_completion_flag);
  ASSERT_TRUE(transfer);
  SubmitLibusbTransferAndWaitForCompletion(*transfer, transfer_completion_flag);

  // Assert.
  EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_ERROR);
  EXPECT_EQ(transfer->actual_length, 0);
  // Attempting to cancel a failed transfer fails.
  EXPECT_NE(libusb()->LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);

  // Cleanup:
  libusb()->LibusbFreeTransfer(transfer);
}

// Test the cancellation of an asynchronous input control transfer.
//
// This test also has other slight variations compared to the previous ones: it
// uses the `LIBUSB_TRANSFER_FREE_TRANSFER` flag and the old
// `LibusbHandleEvents()` API.
TEST_P(LibusbJsProxyWithDeviceTest, AsyncInputControlTransferCancellation) {
  constexpr int kDataLengthRequested = 100;

  // Arrange. Set up the expectation for the request message. We won't reply to
  // this message (until after we cancel the transfer).
  auto waiter = global_context()->CreateRequestWaiter(
      "libusb", "controlTransfer",
      MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                             /*request_type=*/"standard",
                                             kDataLengthRequested));

  // Act.
  std::vector<uint8_t> setup =
      MakeLibusbInputControlTransferSetup(kDataLengthRequested);
  int transfer_completion_flag = 0;
  libusb_transfer* transfer =
      InitLibusbControlTransfer(/*timeout=*/0, setup, transfer_completion_flag);
  ASSERT_TRUE(transfer);
  // In this test we also verify the automatic deallocation of the transfer. We
  // need to use a custom callback as we can only inspect the transfer state in
  // callback (the transfer is destroyed afterwards).
  transfer->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
  transfer->callback = [](libusb_transfer* transfer) {
    ASSERT_TRUE(transfer);
    EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_CANCELLED);
    EXPECT_EQ(transfer->actual_length, 0);
    // Execute the default action that sets `transfer_completion_flag`.
    OnLibusbAsyncTransferCompleted(transfer);
  };

  EXPECT_EQ(libusb()->LibusbSubmitTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completion_flag);

  EXPECT_EQ(libusb()->LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completion_flag);
  // Second attempt to cancel a transfer fails.
  EXPECT_NE(libusb()->LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);
  EXPECT_FALSE(transfer_completion_flag);
  // Let the cancellation propagate.
  WaitForLibusbTransferCompletionViaOldApi(transfer_completion_flag);

  // A reply from the JS side has no effect for the already canceled transfer.
  waiter->Reply(MakeInputTransferFakeJsReply({1, 2, 3}));

  // Nothing to assert here - due to the `LIBUSB_TRANSFER_FREE_TRANSFER` flag
  // the `transfer` is already deallocated here. All assertions are done inside
  // the callback.
}

// Test an asynchronous output control transfer successful scenario.
TEST_P(LibusbJsProxyWithDeviceTest, AsyncOutputControlTransfer) {
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context()->WillReplyToRequestWith(
      "libusb", "controlTransfer",
      MakeExpectedOutputControlTransferJsArgs(
          /*recipient=*/"endpoint", /*request_type=*/"standard", kData),
      MakeOutputTransferFakeJsReply());

  // Act.
  std::vector<uint8_t> setup = MakeLibusbOutputControlTransferSetup(kData);
  int transfer_completion_flag = 0;
  libusb_transfer* const transfer =
      InitLibusbControlTransfer(/*timeout=*/0, setup, transfer_completion_flag);
  ASSERT_TRUE(transfer);
  SubmitLibusbTransferAndWaitForCompletion(*transfer, transfer_completion_flag);

  // Assert.
  EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_COMPLETED);
  EXPECT_EQ(transfer->actual_length, static_cast<int>(kData.size()));
  // Attempting to cancel a completed transfer fails.
  EXPECT_NE(libusb()->LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);

  // Cleanup:
  libusb()->LibusbFreeTransfer(transfer);
}

// Test an asynchronous output control transfer when it fails on the JS side.
TEST_P(LibusbJsProxyWithDeviceTest, AsyncOutputControlTransferFailure) {
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange.
  global_context()->WillReplyToRequestWithError(
      "libusb", "controlTransfer",
      MakeExpectedOutputControlTransferJsArgs(
          /*recipient=*/"endpoint", /*request_type=*/"standard", kData),
      "Fake failure");

  // Act.
  std::vector<uint8_t> setup = MakeLibusbOutputControlTransferSetup(kData);
  int transfer_completion_flag = 0;
  libusb_transfer* const transfer =
      InitLibusbControlTransfer(/*timeout=*/0, setup, transfer_completion_flag);
  ASSERT_TRUE(transfer);
  SubmitLibusbTransferAndWaitForCompletion(*transfer, transfer_completion_flag);

  // Assert.
  EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_ERROR);
  EXPECT_EQ(transfer->actual_length, 0);
  // Attempting to cancel a failed transfer fails.
  EXPECT_NE(libusb()->LibusbCancelTransfer(transfer), LIBUSB_SUCCESS);

  // Cleanup:
  libusb()->LibusbFreeTransfer(transfer);
}

// Test that it's not possible to cancel an asynchronous output control transfer
// (only cancelling input transfers is supported by our implementation).
TEST_P(LibusbJsProxyWithDeviceTest, AsyncOutputControlTransferCancellation) {
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};

  // Arrange. Set up the expectation for the request message. We will reply to
  // this message only after attempting to cancel the transfer.
  auto waiter = global_context()->CreateRequestWaiter(
      "libusb", "controlTransfer",
      MakeExpectedOutputControlTransferJsArgs(
          /*recipient=*/"endpoint", /*request_type=*/"standard", kData));

  // Act.
  std::vector<uint8_t> setup = MakeLibusbOutputControlTransferSetup(kData);
  int transfer_completion_flag = 0;
  libusb_transfer* const transfer =
      InitLibusbControlTransfer(/*timeout=*/0, setup, transfer_completion_flag);
  ASSERT_TRUE(transfer);
  // In this test we also verify the automatic deallocation of the transfer. We
  // need to use a custom callback as we can only inspect the transfer state in
  // callback (the transfer is destroyed afterwards).
  transfer->flags |= LIBUSB_TRANSFER_FREE_TRANSFER;
  transfer->callback = [](libusb_transfer* transfer) {
    ASSERT_TRUE(transfer);
    EXPECT_EQ(transfer->status, LIBUSB_TRANSFER_COMPLETED);
    // Check `actual_length` equals `kData.size()` (we can't pass it explicitly
    // as we're a captureless lambda).
    EXPECT_EQ(transfer->actual_length, 6);
    // Execute the default action that sets `transfer_completion_flag`.
    OnLibusbAsyncTransferCompleted(transfer);
  };
  EXPECT_EQ(libusb()->LibusbSubmitTransfer(transfer), LIBUSB_SUCCESS);

  // Wait for the JS request to be sent.
  waiter->Wait();
  EXPECT_FALSE(transfer_completion_flag);

  // Attempt to cancel the transfer - this is expected to fail.
  EXPECT_EQ(libusb()->LibusbCancelTransfer(transfer), LIBUSB_ERROR_NOT_FOUND);

  // Simulate a successful transfer reply from the JS side.
  waiter->Reply(MakeOutputTransferFakeJsReply());
  EXPECT_FALSE(transfer_completion_flag);

  // Let the fake JS result propagate.
  WaitForLibusbTransferCompletion(transfer_completion_flag);

  // Nothing to assert here - due to the `LIBUSB_TRANSFER_FREE_TRANSFER` flag
  // the `transfer` is already deallocated here. All assertions are done inside
  // the callback.
}

// Test the scenario with making another input control transfer with the same
// parameters as a previously canceled transfer. In this scenario, the JS reply
// that was originally sent to the first transfer's request is "rerouted" to the
// second transfer.
TEST_P(LibusbJsProxyWithDeviceTest, AsyncInputControlTransferDataRerouting) {
  const std::vector<uint8_t> kData = {1, 2, 3, 4, 5, 6};
  // Let two transfers use different timeouts and requested data sizes: these
  // parameters shouldn't affect the "rerouting" of request results.
  const size_t kDataLengthRequested[2] = {100, 200};
  constexpr int kTimeoutsMs[2] = {300000, 400000};

  // Arrange: expect two C++-to-JS transfer requests. We don't schedule replies
  // to these requests initially.
  std::unique_ptr<TestingGlobalContext::Waiter> waiters[2];
  for (int i = 0; i < 2; ++i) {
    waiters[i] = global_context()->CreateRequestWaiter(
        "libusb", "controlTransfer",
        MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                               /*request_type=*/"standard",
                                               kDataLengthRequested[i]));
  }

  // Act.
  // Send the first transfer request and cancel it immediately after it's sent.
  // Enclose this block into curly brackets, so that the test verifies that none
  // of the variables is touched when the second transfer runs later.
  {
    std::vector<uint8_t> setup1 =
        MakeLibusbInputControlTransferSetup(kDataLengthRequested[0]);
    int transfer1_completion_flag = 0;
    libusb_transfer* const transfer1 = InitLibusbControlTransfer(
        /*timeout=*/kTimeoutsMs[0], setup1, transfer1_completion_flag);
    ASSERT_TRUE(transfer1);
    EXPECT_EQ(libusb()->LibusbSubmitTransfer(transfer1), LIBUSB_SUCCESS);
    waiters[0]->Wait();
    EXPECT_EQ(libusb()->LibusbCancelTransfer(transfer1), LIBUSB_SUCCESS);
    WaitForLibusbTransferCompletion(transfer1_completion_flag);
    EXPECT_EQ(transfer1->status, LIBUSB_TRANSFER_CANCELLED);
    libusb()->LibusbFreeTransfer(transfer1);
  }
  // Send the second transfer request.
  std::vector<uint8_t> setup2 =
      MakeLibusbInputControlTransferSetup(kDataLengthRequested[1]);
  int transfer2_completion_flag = 0;
  libusb_transfer* const transfer2 = InitLibusbControlTransfer(
      /*timeout=*/kTimeoutsMs[1], setup2, transfer2_completion_flag);
  ASSERT_TRUE(transfer2);
  EXPECT_EQ(libusb()->LibusbSubmitTransfer(transfer2), LIBUSB_SUCCESS);
  // Simulate a JS reply to the request initiated by the first transfer.
  waiters[0]->Reply(MakeInputTransferFakeJsReply(kData));
  // Wait until the second transfer receives the "rerouted" JS reply.
  WaitForLibusbTransferCompletion(transfer2_completion_flag);

  // Assert.
  EXPECT_EQ(transfer2->status, LIBUSB_TRANSFER_COMPLETED);
  EXPECT_EQ(transfer2->actual_length, static_cast<int>(kData.size()));
  EXPECT_EQ(std::vector<uint8_t>(
                setup2.begin() + LIBUSB_CONTROL_SETUP_SIZE,
                setup2.begin() + LIBUSB_CONTROL_SETUP_SIZE + kData.size()),
            kData);

  // Cleanup:
  libusb()->LibusbFreeTransfer(transfer2);
}

// Verify that input transfers receive results in the FIFO order: the
// first-submitted transfer gets the first-received reply from JS, etc.
TEST_P(LibusbJsProxyWithDeviceTest, InputTransfersFifoOrdering) {
  constexpr int kTransferCount = 100;
  static_assert(kTransferCount < 256, "unexpected kTransferCount");
  constexpr int kDataSizeBytes = 1;
  static_assert(kDataSizeBytes > 0, "unexpected kDataSizeBytes");
  constexpr int kTimeoutsMs = 100000;

  // Arrange: prepare waiters for expected JS requests.
  std::vector<std::unique_ptr<TestingGlobalContext::Waiter>> js_request_waiters(
      kTransferCount);
  for (int i = 0; i < kTransferCount; ++i) {
    js_request_waiters[i] = global_context()->CreateRequestWaiter(
        "libusb", "controlTransfer",
        MakeExpectedInputControlTransferJsArgs(/*recipient=*/"endpoint",
                                               /*request_type=*/"standard",
                                               kDataSizeBytes));
  }
  // Create and submit transfers.
  std::vector<int> transfer_completion_flags(kTransferCount);
  std::vector<std::vector<uint8_t>> transfer_buffers(kTransferCount);
  std::vector<libusb_transfer*> transfers(kTransferCount);
  for (int i = 0; i < kTransferCount; ++i) {
    transfer_buffers[i] = MakeLibusbInputControlTransferSetup(kDataSizeBytes);
    transfers[i] = InitLibusbControlTransfer(kTimeoutsMs, transfer_buffers[i],
                                             transfer_completion_flags[i]);
    EXPECT_EQ(libusb()->LibusbSubmitTransfer(transfers[i]), LIBUSB_SUCCESS);
  }
  // Prepare fake transfer replies. Make them all different, so that the test
  // can verify the ordering of the received replies.
  std::vector<std::vector<uint8_t>> replies(kTransferCount);
  for (int i = 0; i < kTransferCount; ++i) {
    replies[i].resize(kDataSizeBytes);
    replies[i][0] = static_cast<uint8_t>(i);
  }

  // Act: simulate JS request replies.
  for (int i = 0; i < kTransferCount; ++i) {
    js_request_waiters[i]->Reply(MakeInputTransferFakeJsReply(replies[i]));
  }
  // Wait until transfers are resolved, in the expected order.
  for (int i = 0; i < kTransferCount; ++i) {
    WaitForLibusbTransferCompletion(transfer_completion_flags[i]);
    for (int j = i + 1; j < kTransferCount; ++j) {
      EXPECT_FALSE(transfer_completion_flags[j]);
    }
  }

  // Assert: verify the transfers received replies in the expected order.
  for (int i = 0; i < kTransferCount; ++i) {
    const auto& buffer = transfer_buffers[i];
    EXPECT_EQ(std::vector<uint8_t>(
                  buffer.begin() + LIBUSB_CONTROL_SETUP_SIZE,
                  buffer.begin() + LIBUSB_CONTROL_SETUP_SIZE + kDataSizeBytes),
              replies[i]);
  }

  // Cleanup:
  for (int i = 0; i < kTransferCount; ++i) {
    libusb()->LibusbFreeTransfer(transfers[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ,
    LibusbJsProxyWithDeviceTest,
    testing::Values(WrapperTestParam::kWithoutTracingWrapper,
                    WrapperTestParam::kWithTracingWrapper),
    PrintWrapperTestParam);

}  // namespace google_smart_card
