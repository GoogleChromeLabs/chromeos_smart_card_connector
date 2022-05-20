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

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/requesting/request_result.h>

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

namespace {

class FakeGlobalContext final : public GlobalContext {
 public:
  void PostMessageToJs(Value /*message*/) override {}
  bool IsMainEventLoopThread() const override { return true; }
  void ShutDown() override {}
};

class LibusbJsProxyTest : public ::testing::Test {
 protected:
  LibusbJsProxyTest() = default;
  ~LibusbJsProxyTest() override = default;

  FakeGlobalContext global_context_;
  TypedMessageRouter typed_message_router_;
  LibusbJsProxy libusb_js_proxy_{&global_context_, &typed_message_router_};
};

}  // namespace

TEST_F(LibusbJsProxyTest, ContextsCreation) {
  ASSERT_EQ(LIBUSB_SUCCESS, libusb_js_proxy_.LibusbInit(nullptr));

  // Initializing a default context for the second time doesn't do anything
  ASSERT_EQ(LIBUSB_SUCCESS, libusb_js_proxy_.LibusbInit(nullptr));

  libusb_context* context_1 = nullptr;
  ASSERT_EQ(LIBUSB_SUCCESS, libusb_js_proxy_.LibusbInit(&context_1));
  EXPECT_TRUE(context_1);

  libusb_context* context_2 = nullptr;
  ASSERT_EQ(LIBUSB_SUCCESS, libusb_js_proxy_.LibusbInit(&context_2));
  EXPECT_TRUE(context_2);
  EXPECT_NE(context_1, context_2);

  libusb_js_proxy_.LibusbExit(context_1);
  libusb_js_proxy_.LibusbExit(context_2);
  libusb_js_proxy_.LibusbExit(nullptr);
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

TEST_F(LibusbJsProxyTest, DevicesListingWithFailure) {
  EXPECT_CALL(*chrome_usb_api_bridge, GetDevices(_))
      .WillOnce(InvokeWithoutArgs([]() {
        return RequestResult<chrome_usb::GetDevicesResult>::CreateFailed(
            "fake failure");
      }));

  libusb_device** device_list;
  ASSERT_EQ(LIBUSB_ERROR_OTHER,
            libusb_js_proxy->LibusbGetDeviceList(nullptr, &device_list));
}

TEST_F(LibusbJsProxyTest, DevicesListingWithNoItems) {
  EXPECT_CALL(*chrome_usb_api_bridge, GetDevices(_))
      .WillOnce(InvokeWithoutArgs([]() {
        return RequestResult<chrome_usb::GetDevicesResult>::CreateSuccessful(
            chrome_usb::GetDevicesResult());
      }));

  libusb_device** device_list = nullptr;
  ASSERT_EQ(0, libusb_js_proxy->LibusbGetDeviceList(nullptr, &device_list));
  ASSERT_TRUE(device_list);
  ASSERT_FALSE(device_list[0]);

  libusb_js_proxy->LibusbFreeDeviceList(device_list, true);
}

TEST_F(LibusbJsProxyTest, DevicesListingWithTwoItems) {
  chrome_usb::GetDevicesResult chrome_usb_get_devices_result;
  chrome_usb::Device chrome_usb_device_1;
  chrome_usb_device_1.device = 0;
  chrome_usb_device_1.vendor_id = 0;
  chrome_usb_device_1.product_id = 0;
  chrome_usb_get_devices_result.devices.push_back(chrome_usb_device_1);
  chrome_usb::Device chrome_usb_device_2;
  chrome_usb_device_2.device = 1;
  chrome_usb_device_2.vendor_id = 0;
  chrome_usb_device_2.product_id = 0;
  chrome_usb_get_devices_result.devices.push_back(chrome_usb_device_2);
  EXPECT_CALL(*chrome_usb_api_bridge, GetDevices(_))
      .WillOnce(InvokeWithoutArgs([&]() {
        return RequestResult<chrome_usb::GetDevicesResult>::CreateSuccessful(
            chrome_usb_get_devices_result);
      }));

  libusb_device** device_list = nullptr;
  ASSERT_EQ(2, libusb_js_proxy->LibusbGetDeviceList(nullptr, &device_list));
  ASSERT_TRUE(device_list);
  ASSERT_TRUE(device_list[0]);
  ASSERT_TRUE(device_list[1]);
  ASSERT_NE(device_list[0], device_list[1]);
  ASSERT_FALSE(device_list[2]);
  libusb_js_proxy->LibusbFreeDeviceList(device_list, true);
}

// TODO(emaxx): Add a test on referencing/unreferencing devices

// TODO(emaxx): Add a test on obtaining device descriptors and attributes

// TODO(emaxx): Add a test on resetting a device

namespace {

class LibusbJsProxyWithFakeDeviceTest : public LibusbJsProxyTest {
 public:
  LibusbJsProxyWithFakeDeviceTest() : device(nullptr), device_handle(nullptr) {
    chrome_usb_device.device = 1;
    chrome_usb_device.vendor_id = 2;
    chrome_usb_device.product_id = 3;
    chrome_usb_device.version = 4;
    chrome_usb_device.product_name = "product";
    chrome_usb_device.manufacturer_name = "manufacturer";
    chrome_usb_device.serial_number = "serial";

    chrome_usb_connection_handle.handle = 1;
    chrome_usb_connection_handle.vendor_id = chrome_usb_device.vendor_id;
    chrome_usb_connection_handle.product_id = chrome_usb_device.product_id;
  }

 protected:
  void SetUp() override {
    LibusbJsProxyTest::SetUp();

    ASSERT_EQ(LIBUSB_SUCCESS, libusb_js_proxy->LibusbInit(nullptr));
    SetUpMocksForFakeDevice();
    ObtainLibusbDevice();
    ObtainLibusbDeviceHandle();
  }

  void TearDown() override {
    libusb_js_proxy->LibusbClose(device_handle);
    device_handle = nullptr;
    libusb_js_proxy->LibusbUnrefDevice(device);
    device = nullptr;
    libusb_js_proxy->LibusbExit(nullptr);

    LibusbJsProxyTest::TearDown();
  }

  chrome_usb::Device chrome_usb_device;
  chrome_usb::ConnectionHandle chrome_usb_connection_handle;
  libusb_device* device;
  libusb_device_handle* device_handle;

 private:
  void SetUpMocksForFakeDevice() {
    chrome_usb::GetDevicesResult chrome_usb_get_devices_result;
    chrome_usb_get_devices_result.devices.push_back(chrome_usb_device);
    // TODO(emaxx): Add testing of the GetDevices method arguments
    EXPECT_CALL(*chrome_usb_api_bridge, GetDevices(_))
        .WillRepeatedly(InvokeWithoutArgs([=]() {
          return RequestResult<chrome_usb::GetDevicesResult>::CreateSuccessful(
              chrome_usb_get_devices_result);
        }));

    chrome_usb::OpenDeviceResult chrome_usb_open_device_result;
    chrome_usb_open_device_result.connection_handle =
        chrome_usb_connection_handle;
    EXPECT_CALL(*chrome_usb_api_bridge, OpenDevice(chrome_usb_device))
        .WillOnce(InvokeWithoutArgs([=]() {
          return RequestResult<chrome_usb::OpenDeviceResult>::CreateSuccessful(
              chrome_usb_open_device_result);
        }));

    // TODO(emaxx): Add testing that calls of methods OpenDevice and CloseDevice
    // form a valid bracket sequence
    EXPECT_CALL(*chrome_usb_api_bridge,
                CloseDevice(chrome_usb_connection_handle))
        .WillOnce(InvokeWithoutArgs([]() {
          return RequestResult<chrome_usb::CloseDeviceResult>::CreateSuccessful(
              chrome_usb::CloseDeviceResult());
        }));
  }

  void ObtainLibusbDevice() {
    libusb_device** device_list = nullptr;
    EXPECT_EQ(1, libusb_js_proxy->LibusbGetDeviceList(nullptr, &device_list));
    EXPECT_TRUE(device_list);
    EXPECT_TRUE(device_list[0]);
    device = device_list[0];
    libusb_js_proxy->LibusbFreeDeviceList(device_list, false);
  }

  void ObtainLibusbDeviceHandle() {
    EXPECT_EQ(LIBUSB_SUCCESS,
              libusb_js_proxy->LibusbOpen(device, &device_handle));
    EXPECT_TRUE(device_handle);
  }
};

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

// Test a simple synchronous control transfer.
//
// The transfer request is resolved immediately on the same thread that
// initiated the transfer.
TEST_P(LibusbJsProxySingleTransferTest, SyncControlTransfer) {
  SetUpMockForSyncControlTransfer(GetParam().transfer_index,
                                  GetParam().is_transfer_output);
  TestSyncControlTransfer(GetParam().transfer_index,
                          GetParam().is_transfer_output);
}

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

// Test the correctness of work of multiple threads issuing a sequence of
// synchronous transfer requests.
//
// Each transfer request is resolved immediately on the same thread that
// initiated the transfer.
TEST_F(LibusbJsProxyTransfersTest, SyncControlTransfersWithMultiThreading) {
  // A high number of transfers increases the chances of catching a bug, but the
  // constant is lower in the Debug mode to avoid running too long.
  const size_t kMaxTransferIndex =
#ifdef NDEBUG
      1000
#else
      100
#endif
      ;
  const size_t kThreadCount = 10;

  for (size_t index = 0; index <= kMaxTransferIndex; ++index) {
    for (bool is_transfer_output : kBoolValues)
      SetUpMockForSyncControlTransfer(index, is_transfer_output);
  }

  std::vector<std::thread> threads;
  for (size_t thread_index = 0; thread_index < kThreadCount; ++thread_index) {
    threads.emplace_back([this, thread_index] {
      for (size_t transfer_index = thread_index;
           transfer_index <= kMaxTransferIndex;
           transfer_index += kThreadCount) {
        for (bool is_transfer_output : kBoolValues)
          TestSyncControlTransfer(transfer_index, is_transfer_output);
      }
    });
  }
  for (size_t thread_index = 0; thread_index < kThreadCount; ++thread_index)
    threads[thread_index].join();
}

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
