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

#include <google_smart_card_common/messaging/typed_message_router.h>

#include <string>
#include <thread>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/messaging/typed_message_listener.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>

// Google Mock doesn't provide the C++11 "override" specifier for the mock
// method definitions
#pragma GCC diagnostic ignored "-Winconsistent-missing-override"

using testing::_;
using testing::Eq;
using testing::ResultOf;
using testing::Return;

namespace google_smart_card {

namespace {

constexpr char kSampleType1[] = "sample type 1";
constexpr char kSampleType2[] = "sample type 2";

class MockTypedMessageListener final : public TypedMessageListener {
 public:
  explicit MockTypedMessageListener(const std::string& message_type)
      : message_type_(message_type) {}

  std::string GetListenedMessageType() const override { return message_type_; }

  bool OnTypedMessageReceived(Value value) override {
    return OnTypedMessageReceivedImpl(value);
  }

  // Mock a method that takes the value as const-ref, since the currently used
  // version of Googletest doesn't support mocking functions with move-only
  // arguments.
  MOCK_METHOD1(OnTypedMessageReceivedImpl, bool(const Value&));

 private:
  std::string message_type_;
};

std::string MakeSampleData(int index) {
  return FormatPrintfTemplate("sample value #%d", index);
}

std::string GetMessageDataString(const Value& data) {
  if (!data.is_string()) {
    ADD_FAILURE() << "Value is not a string";
    return "";
  }
  return data.GetString();
}

Value MakeTypedMessageValue(const std::string& type, const std::string& data) {
  TypedMessage typed_message;
  typed_message.type = type;
  typed_message.data = Value(data);
  Value typed_message_value;
  EXPECT_TRUE(ConvertToValue(std::move(typed_message), &typed_message_value));
  return typed_message_value;
}

}  // namespace

TEST(MessagingTypedMessageRouterTest, Basic) {
  TypedMessageRouter router;
  MockTypedMessageListener listener_1(kSampleType1);
  MockTypedMessageListener listener_2(kSampleType2);

  // Initially, the router contains no route, so no listeners are invoked
  EXPECT_FALSE(router.OnMessageReceived(
      MakeTypedMessageValue(kSampleType1, MakeSampleData(1))));
  EXPECT_FALSE(router.OnMessageReceived(
      MakeTypedMessageValue(kSampleType2, MakeSampleData(2))));

  // After the first listener is registered, it is invoked by the router on the
  // corresponding message receival
  router.AddRoute(&listener_1);
  EXPECT_CALL(listener_1, OnTypedMessageReceivedImpl(ResultOf(
                              GetMessageDataString, Eq(MakeSampleData(3)))))
      .WillOnce(Return(true));
  EXPECT_TRUE(router.OnMessageReceived(
      MakeTypedMessageValue(kSampleType1, MakeSampleData(3))));
  EXPECT_FALSE(router.OnMessageReceived(
      MakeTypedMessageValue(kSampleType2, MakeSampleData(4))));

  // When the listener returns false, the router returns false too
  EXPECT_CALL(listener_1, OnTypedMessageReceivedImpl(ResultOf(
                              GetMessageDataString, Eq(MakeSampleData(5)))))
      .WillOnce(Return(false));
  EXPECT_FALSE(router.OnMessageReceived(
      MakeTypedMessageValue(kSampleType1, MakeSampleData(5))));

  // After the second listener is registered, it is invoked by the router on the
  // corresponding message receival
  router.AddRoute(&listener_2);
  EXPECT_CALL(listener_2, OnTypedMessageReceivedImpl(ResultOf(
                              GetMessageDataString, Eq(MakeSampleData(6)))))
      .WillOnce(Return(true));
  EXPECT_TRUE(router.OnMessageReceived(
      MakeTypedMessageValue(kSampleType2, MakeSampleData(6))));

  // After the first listener is removed, it is no more invoked by the router
  router.RemoveRoute(&listener_1);
  EXPECT_FALSE(router.OnMessageReceived(
      MakeTypedMessageValue(kSampleType1, MakeSampleData(7))));
}

TEST(MessagingTypedMessageRouterTest, MultiThreading) {
  const int kIterationCount = 100 * 1000;

  TypedMessageRouter router;

  MockTypedMessageListener listener_1(kSampleType1);
  EXPECT_CALL(listener_1, OnTypedMessageReceivedImpl(_))
      .WillRepeatedly(Return(true));
  MockTypedMessageListener listener_2(kSampleType2);
  EXPECT_CALL(listener_2, OnTypedMessageReceivedImpl(_))
      .WillRepeatedly(Return(true));

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
    for (int iteration = 0; iteration < kIterationCount; ++iteration) {
      Value message_1 = MakeTypedMessageValue(kSampleType1, MakeSampleData(0));
      Value message_2 = MakeTypedMessageValue(kSampleType2, MakeSampleData(0));
      router.OnMessageReceived(std::move(message_1));
      router.OnMessageReceived(std::move(message_2));
    }
  });

  route_1_operating_thread.join();
  route_2_operating_thread.join();
  message_pushing_thread.join();
}

}  // namespace google_smart_card
