// Copyright 2016 Google Inc. All Rights Reserved.
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

#include <string>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ppapi/cpp/var.h>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/messaging/typed_message_router.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

// Google Mock doesn't provide the C++11 "override" specifier for the mock
// method definitions
#pragma GCC diagnostic ignored "-Winconsistent-missing-override"

const char kSampleType1[] = "sample type 1";
const char kSampleType2[] = "sample type 2";
const char kSampleDataFormat[] = "sample value #%1%";

using testing::_;
using testing::Eq;
using testing::ResultOf;
using testing::Return;

namespace google_smart_card {

namespace {

class MockTypedMessageListener final : public TypedMessageListener {
 public:
  explicit MockTypedMessageListener(const std::string& message_type)
      : message_type_(message_type) {}

  std::string GetListenedMessageType() const override {
    return message_type_;
  }

  MOCK_METHOD1(OnTypedMessageReceived, bool(const pp::Var&));

 private:
  std::string message_type_;
};

std::string MakeSampleData(int index) {
  return FormatBoostFormatTemplate(kSampleDataFormat, index);
}

std::string GetMessageDataString(const pp::Var& data) {
  return VarAs<std::string>(data);
}

}  // namespace

TEST(MessagingTypedMessageRouterTest, Basic) {
  TypedMessageRouter router;
  MockTypedMessageListener listener_1(kSampleType1);
  MockTypedMessageListener listener_2(kSampleType2);

  // Initially, the router contains no route, so no listeners are invoked
  EXPECT_FALSE(router.OnMessageReceived(MakeTypedMessage(
      kSampleType1, MakeSampleData(1))));
  EXPECT_FALSE(router.OnMessageReceived(MakeTypedMessage(
      kSampleType2, MakeSampleData(2))));

  // After the first listener is registered, it is invoked by the router on the
  // corresponding message receival
  router.AddRoute(&listener_1);
  EXPECT_CALL(
      listener_1,
      OnTypedMessageReceived(
          ResultOf(GetMessageDataString, Eq(MakeSampleData(3)))))
      .WillOnce(Return(true));
  EXPECT_TRUE(router.OnMessageReceived(MakeTypedMessage(
      kSampleType1, MakeSampleData(3))));
  EXPECT_FALSE(router.OnMessageReceived(MakeTypedMessage(
      kSampleType2, MakeSampleData(4))));

  // When the listener returns false, the router returns false too
  EXPECT_CALL(
      listener_1,
      OnTypedMessageReceived(
          ResultOf(GetMessageDataString, Eq(MakeSampleData(5)))))
      .WillOnce(Return(false));
  EXPECT_FALSE(router.OnMessageReceived(MakeTypedMessage(
      kSampleType1, MakeSampleData(5))));

  // After the second listener is registered, it is invoked by the router on the
  // corresponding message receival
  router.AddRoute(&listener_2);
  EXPECT_CALL(
      listener_2,
      OnTypedMessageReceived(
          ResultOf(GetMessageDataString, Eq(MakeSampleData(6)))))
      .WillOnce(Return(true));
  EXPECT_TRUE(router.OnMessageReceived(MakeTypedMessage(
      kSampleType2, MakeSampleData(6))));

  // After the first listener is removed, it is no more invoked by the router
  router.RemoveRoute(&listener_1);
  EXPECT_FALSE(router.OnMessageReceived(MakeTypedMessage(
      kSampleType1, MakeSampleData(7))));
}

TEST(MessagingTypedMessageRouterTest, MultiThreading) {
  const int kIterationCount = 100 * 1000;

  TypedMessageRouter router;

  MockTypedMessageListener listener_1(kSampleType1);
  EXPECT_CALL(listener_1, OnTypedMessageReceived(_)).WillRepeatedly(
      Return(true));
  MockTypedMessageListener listener_2(kSampleType2);
  EXPECT_CALL(listener_2, OnTypedMessageReceived(_)).WillRepeatedly(
      Return(true));

  std::thread route_1_operating_thread([&router, &listener_1] {
    for (int iteration = 0; iteration < kIterationCount; ++iteration) {
      router.AddRoute(&listener_1);
      router.RemoveRoute(&listener_1);
    }
  });
  std::thread route_2_operating_thread([&router, &listener_2] {
    for (int iteration = 0; iteration < kIterationCount; ++iteration) {
      router.AddRoute(&listener_2);
      router.RemoveRoute(&listener_2);
    }
  });
  std::thread message_pushing_thread([&router] {
    const pp::Var message_1 = MakeTypedMessage(kSampleType1, MakeSampleData(0));
    const pp::Var message_2 = MakeTypedMessage(kSampleType2, MakeSampleData(0));
    for (int iteration = 0; iteration < kIterationCount; ++iteration) {
      router.OnMessageReceived(message_1);
      router.OnMessageReceived(message_2);
    }
  });

  route_1_operating_thread.join();
  route_2_operating_thread.join();
  message_pushing_thread.join();
}

}  // namespace google_smart_card
