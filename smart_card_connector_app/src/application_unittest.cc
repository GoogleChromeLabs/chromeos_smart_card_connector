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

#include <sys/mount.h>

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/multi_string.h>
#include <google_smart_card_common/unique_ptr_utils.h>

#include "common/cpp/src/public/testing_global_context.h"
#include "common/cpp/src/public/value_builder.h"

#include <winscard.h>

#include <sys/types.h>
#include <dirent.h>

using testing::ElementsAre;

namespace google_smart_card {

class SmartCardConnectorApplicationTest : public ::testing::Test {
 protected:
  SmartCardConnectorApplicationTest() {
#ifdef __native_client__
    GOOGLE_SMART_CARD_CHECK(::umount("/") == 0);
    GOOGLE_SMART_CARD_CHECK(
        ::mount("/", "/", "httpfs", 0, "manifest=/nacl_io_manifest.txt") == 0);
#endif

    for (int i = 0; i < 10; ++i) {
      global_context_.WillReplyToRequestWith(
          "libusb", "listDevices",
          /*arguments=*/Value(Value::Type::kArray),
          /*result_to_reply_with=*/
          ArrayValueBuilder()
              .Add(DictValueBuilder()
                       .Add("deviceId", 123)
                       .Add("vendorId", 0x08E6)
                       .Add("productId", 0x3437)
                       .Get())
              .Get());
      global_context_.WillReplyToRequestWith(
          "libusb", "getConfigurations",
          /*arguments=*/ArrayValueBuilder().Add(123).Get(),
          /*result_to_reply_with=*/
          ArrayValueBuilder()
              .Add(
                  DictValueBuilder()
                      .Add("active", true)
                      .Add("configurationValue", 1)
                      .Add("extraData", std::vector<uint8_t>())
                      .Add(
                          "interfaces",
                          ArrayValueBuilder()
                              .Add(
                                  DictValueBuilder()
                                      .Add("interfaceNumber", 0)
                                      .Add("interfaceClass", 0xB)
                                      .Add("interfaceSubclass", 0)
                                      .Add("interfaceProtocol", 0)
                                      .Add("extraData",
                                           std::vector<uint8_t>{
                                               0x36, 0x21, 0x01, 0x01, 0x00,
                                               0x07, 0x03, 0x00, 0x00, 0x00,
                                               0xC0, 0x12, 0x00, 0x00, 0xC0,
                                               0x12, 0x00, 0x00, 0x00, 0x67,
                                               0x32, 0x00, 0x00, 0xCE, 0x99,
                                               0x0C, 0x00, 0x35, 0xFE, 0x00,
                                               0x00, 0x00, 0x00, 0x00, 0x00,
                                               0x00, 0x00, 0x00, 0x00, 0x00,
                                               0x30, 0x02, 0x01, 0x00, 0x0F,
                                               0x01, 0x00, 0x00, 0x00, 0x00,
                                               0x00, 0x00, 0x00, 0x01})
                                      .Add(
                                          "endpoints",
                                          ArrayValueBuilder()
                                              .Add(
                                                  DictValueBuilder()
                                                      .Add("endpointAddress", 1)
                                                      .Add("direction", "out")
                                                      .Add("type", "bulk")
                                                      .Add("extraData",
                                                           std::vector<
                                                               uint8_t>())
                                                      .Add("maxPacketSize", 64)
                                                      .Get())
                                              .Add(DictValueBuilder()
                                                       .Add("endpointAddress",
                                                            0x82)
                                                       .Add("direction", "in")
                                                       .Add("type", "bulk")
                                                       .Add("extraData",
                                                            std::vector<
                                                                uint8_t>())
                                                       .Add("maxPacketSize", 64)
                                                       .Get())
                                              .Add(DictValueBuilder()
                                                       .Add("endpointAddress",
                                                            0x83)
                                                       .Add("direction", "in")
                                                       .Add("type", "interrupt")
                                                       .Add("extraData",
                                                            std::vector<
                                                                uint8_t>())
                                                       .Add("maxPacketSize", 8)
                                                       .Get())
                                              .Get())
                                      .Get())
                              .Get())
                      .Get())
              .Get());
    }
    global_context_.WillReplyToRequestWith(
        "libusb", "openDeviceHandle",
        /*arguments=*/ArrayValueBuilder().Add(123).Get(),
        /*result_to_reply_with=*/Value(456));
    global_context_.WillReplyToRequestWith(
        "libusb", "claimInterface",
        /*arguments=*/ArrayValueBuilder().Add(123).Add(456).Add(0).Get(),
        /*result_to_reply_with=*/Value(Value::Type::kArray));
    global_context_.WillReplyToRequestWith(
        "libusb", "controlTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("index", 0)
                     .Add("lengthToReceive", 212)
                     .Add("recipient", "interface")
                     .Add("request", 3)
                     .Add("requestType", "class")
                     .Add("value", 0)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder()
            .Add(
                "receivedData",
                std::vector<uint8_t>(
                    {0x67, 0x32, 0x00, 0x00, 0xCE, 0x64, 0x00, 0x00, 0x9D, 0xC9,
                     0x00, 0x00, 0x3A, 0x93, 0x01, 0x00, 0x74, 0x26, 0x03, 0x00,
                     0xE7, 0x4C, 0x06, 0x00, 0xCE, 0x99, 0x0C, 0x00, 0xD7, 0x5C,
                     0x02, 0x00, 0x11, 0xF0, 0x03, 0x00, 0x34, 0x43, 0x00, 0x00,
                     0x69, 0x86, 0x00, 0x00, 0xD1, 0x0C, 0x01, 0x00, 0xA2, 0x19,
                     0x02, 0x00, 0x45, 0x33, 0x04, 0x00, 0x8A, 0x66, 0x08, 0x00,
                     0x0B, 0xA0, 0x02, 0x00, 0x73, 0x30, 0x00, 0x00, 0xE6, 0x60,
                     0x00, 0x00, 0xCC, 0xC1, 0x00, 0x00, 0x99, 0x83, 0x01, 0x00,
                     0x32, 0x07, 0x03, 0x00, 0x63, 0x0E, 0x06, 0x00, 0xB3, 0x22,
                     0x01, 0x00, 0x7F, 0xE4, 0x01, 0x00, 0x06, 0x50, 0x01, 0x00,
                     0x36, 0x97, 0x00, 0x00, 0x04, 0xFC, 0x00, 0x00, 0x53, 0x28,
                     0x00, 0x00, 0xA5, 0x50, 0x00, 0x00, 0x4A, 0xA1, 0x00, 0x00,
                     0x95, 0x42, 0x01, 0x00, 0x29, 0x85, 0x02, 0x00, 0xF8, 0x78,
                     0x00, 0x00, 0x3E, 0x49, 0x00, 0x00, 0x7C, 0x92, 0x00, 0x00,
                     0xF8, 0x24, 0x01, 0x00, 0xF0, 0x49, 0x02, 0x00, 0xE0, 0x93,
                     0x04, 0x00, 0xC0, 0x27, 0x09, 0x00, 0x74, 0xB7, 0x01, 0x00,
                     0x6C, 0xDC, 0x02, 0x00, 0xD4, 0x30, 0x00, 0x00, 0xA8, 0x61,
                     0x00, 0x00, 0x50, 0xC3, 0x00, 0x00, 0xA0, 0x86, 0x01, 0x00,
                     0x40, 0x0D, 0x03, 0x00, 0x80, 0x1A, 0x06, 0x00, 0x48, 0xE8,
                     0x01, 0x00, 0xBA, 0xDB, 0x00, 0x00, 0x36, 0x6E, 0x01, 0x00,
                     0x24, 0xF4, 0x00, 0x00, 0xDD, 0x6D, 0x00, 0x00, 0x1B, 0xB7,
                     0x00, 0x00}))
            .Get());
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 1)
                     .Add("dataToSend",
                          std::vector<uint8_t>({0x65, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x00, 0x00, 0x00, 0x00}))
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x82)
                     .Add("lengthToReceive", 10)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder()
            .Add("receivedData",
                 std::vector<uint8_t>({0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x02, 0x00, 0x00}))
            .Get());
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 1)
                     .Add("dataToSend",
                          std::vector<uint8_t>({0x65, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x01, 0x00, 0x00, 0x00}))
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x82)
                     .Add("lengthToReceive", 10)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder()
            .Add("receivedData",
                 std::vector<uint8_t>({0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
                                       0x02, 0x00, 0x00}))
            .Get());
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 1)
                     .Add("dataToSend", std::vector<uint8_t>(
                                            {0x6B, 0x01, 0x00, 0x00, 0x00, 0x00,
                                             0x02, 0x00, 0x00, 0x00, 0x6A}))
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x82)
                     .Add("lengthToReceive", 31)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder()
            .Add("receivedData",
                 std::vector<uint8_t>({0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
                                       0x42, 0x0A, 0x00}))
            .Get());
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 1)
                     .Add("dataToSend",
                          std::vector<uint8_t>({0x65, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x03, 0x00, 0x00, 0x00}))
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x82)
                     .Add("lengthToReceive", 10)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder()
            .Add("receivedData",
                 std::vector<uint8_t>({0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
                                       0x02, 0x00, 0x00}))
            .Get());
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 1)
                     .Add("dataToSend",
                          std::vector<uint8_t>({0x65, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x04, 0x00, 0x00, 0x00}))
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x82)
                     .Add("lengthToReceive", 10)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder()
            .Add("receivedData",
                 std::vector<uint8_t>({0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
                                       0x02, 0x00, 0x00}))
            .Get());
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 1)
                     .Add("dataToSend",
                          std::vector<uint8_t>({0x65, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x05, 0x00, 0x00, 0x00}))
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x82)
                     .Add("lengthToReceive", 10)
                     .Get())
            .Get(),
        /*result_to_reply_with=*/
        DictValueBuilder()
            .Add("receivedData",
                 std::vector<uint8_t>({0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
                                       0x02, 0x00, 0x00}))
            .Get());

    global_context_.WillReplyToRequestWith(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 1)
                     .Add("dataToSend",
                          std::vector<uint8_t>({0x63, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x06, 0x00, 0x00, 0x00}))
                     .Get())
            .Get(),
        /*result_to_reply_with=*/Value(Value::Type::kDictionary));
    waiters_.push_back(global_context_.CreateRequestWaiter(
        "libusb", "bulkTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x82)
                     .Add("lengthToReceive", 10)
                     .Get())
            .Get()));

    waiters_.push_back(global_context_.CreateMessageWaiter("reader_init_add"));
    waiters_.push_back(
        global_context_.CreateMessageWaiter("reader_finish_add"));
    waiters_.push_back(global_context_.CreateRequestWaiter(
        "libusb", "interruptTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x83)
                     .Add("lengthToReceive", 8)
                     .Get())
            .Get()));
    waiters_.push_back(global_context_.CreateRequestWaiter(
        "libusb", "interruptTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x83)
                     .Add("lengthToReceive", 8)
                     .Get())
            .Get()));
    waiters_.push_back(global_context_.CreateRequestWaiter(
        "libusb", "interruptTransfer",
        /*arguments=*/
        ArrayValueBuilder()
            .Add(123)
            .Add(456)
            .Add(DictValueBuilder()
                     .Add("endpointAddress", 0x83)
                     .Add("lengthToReceive", 8)
                     .Get())
            .Get()));

    StartApplication();
  }

  ~SmartCardConnectorApplicationTest() {
    GOOGLE_SMART_CARD_LOG_INFO << "[EMAXX] ~SmartCardConnectorApplicationTest: BEGIN{";
    global_context_.WillReplyToRequestWith(
        "libusb", "releaseInterface",
        /*arguments=*/ArrayValueBuilder().Add(123).Add(456).Add(0).Get(),
        /*result_to_reply_with=*/Value(Value::Type::kArray));
    global_context_.WillReplyToRequestWith(
        "libusb", "closeDeviceHandle",
        /*arguments=*/ArrayValueBuilder().Add(123).Add(456).Get(),
        /*result_to_reply_with=*/Value(Value::Type::kArray));

    application_->ShutDownAndWait();
    GOOGLE_SMART_CARD_LOG_INFO << "[EMAXX] ~SmartCardConnectorApplicationTest: }END";
  }

 private:
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
  TestingGlobalContext global_context_{&typed_message_router_};
  std::vector<std::unique_ptr<TestingGlobalContext::Waiter>> waiters_;
  std::unique_ptr<Application> application_;
};

TEST_F(SmartCardConnectorApplicationTest, SmokeTest) {}

TEST_F(SmartCardConnectorApplicationTest, ListReaders) {
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

TEST_F(SmartCardConnectorApplicationTest, ListReadersFailureSmallerBuffer) {
  SCARDCONTEXT scard_context = 0;
  EXPECT_EQ(SCardEstablishContext(SCARD_SCOPE_SYSTEM, /*pvReserved1=*/nullptr,
                                  /*pvReserved2=*/nullptr, &scard_context),
            SCARD_S_SUCCESS);

  DWORD readers_size = 0;
  EXPECT_EQ(SCardListReaders(scard_context, /*mszGroups=*/nullptr,
                             /*mszReaders=*/nullptr, &readers_size),
            SCARD_S_SUCCESS);
  --readers_size;

  std::string readers_multistring;
  readers_multistring.resize(readers_size);
  EXPECT_EQ(SCardListReaders(scard_context, /*mszGroups=*/nullptr,
                             &readers_multistring[0], &readers_size),
            SCARD_E_INSUFFICIENT_BUFFER);

  EXPECT_EQ(SCardReleaseContext(scard_context), SCARD_S_SUCCESS);
}

}  // namespace google_smart_card
