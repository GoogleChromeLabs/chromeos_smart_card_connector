// Copyright 2020 Google Inc. All Rights Reserved.
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

#include <google_smart_card_common/requesting/remote_call_arguments_conversion.h>

#include <string>

#include <gtest/gtest.h>

#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

namespace {

constexpr char kSomeFunc[] = "someFunc";

enum class SomeEnum { kFirst };

struct SomeStruct {
  int foo;
};

}  // namespace

template <>
EnumValueDescriptor<SomeEnum>::Description
EnumValueDescriptor<SomeEnum>::GetDescription() {
  return Describe("SomeEnum").WithItem(SomeEnum::kFirst, "first");
}

template <>
StructValueDescriptor<SomeStruct>::Description
StructValueDescriptor<SomeStruct>::GetDescription() {
  return Describe("SomeStruct").WithField(&SomeStruct::foo, "foo");
}

TEST(ToRemoteCallRequestConversion, NoArguments) {
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc);
  EXPECT_EQ(payload.function_name, kSomeFunc);
  EXPECT_TRUE(payload.arguments.empty());
}

TEST(ToRemoteCallRequestConversion, BasicArgumentByValue) {
  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, false);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_boolean());
    EXPECT_EQ(payload.arguments[0].GetBoolean(), false);
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, 123);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_integer());
    EXPECT_EQ(payload.arguments[0].GetInteger(), 123);
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, "foo");
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_string());
    EXPECT_EQ(payload.arguments[0].GetString(), "foo");
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, std::string("foo"));
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_string());
    EXPECT_EQ(payload.arguments[0].GetString(), "foo");
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc,
                                               std::vector<int>({1, 10}));
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    const Value& argument0 = payload.arguments[0];
    ASSERT_TRUE(argument0.is_array());
    ASSERT_EQ(argument0.GetArray().size(), 2U);
    ASSERT_TRUE(argument0.GetArray()[0]->is_integer());
    EXPECT_EQ(argument0.GetArray()[0]->GetInteger(), 1);
    ASSERT_TRUE(argument0.GetArray()[1]->is_integer());
    EXPECT_EQ(argument0.GetArray()[1]->GetInteger(), 10);
  }
}

TEST(ToRemoteCallRequestConversion, BasicArgumentByReference) {
  {
    bool boolean_value = false;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_value);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_boolean());
    EXPECT_EQ(payload.arguments[0].GetBoolean(), false);
  }

  {
    std::string string_value = "foo";
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, string_value);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_string());
    EXPECT_EQ(payload.arguments[0].GetString(), "foo");
    // The call shouldn't modify arguments passed by reference.
    EXPECT_EQ(string_value, "foo");
  }
}

TEST(ToRemoteCallRequestConversion, BasicArgumentByConstReference) {
  const bool kBooleanValue = false;
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, kBooleanValue);
  EXPECT_EQ(payload.function_name, kSomeFunc);
  ASSERT_EQ(payload.arguments.size(), 1U);
  ASSERT_TRUE(payload.arguments[0].is_boolean());
  EXPECT_EQ(payload.arguments[0].GetBoolean(), kBooleanValue);
}

TEST(ToRemoteCallRequestConversion, OptionalArgumentByValue) {
  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, optional<int>());
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    EXPECT_TRUE(payload.arguments[0].is_null());
  }

  {
    constexpr int kIntegerValue = 123;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc,
                                               make_optional(kIntegerValue));
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_integer());
    EXPECT_EQ(payload.arguments[0].GetInteger(), kIntegerValue);
  }
}

TEST(ToRemoteCallRequestConversion, OptionalArgumentByReference) {
  {
    optional<bool> boolean_argument;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    EXPECT_TRUE(payload.arguments[0].is_null());
  }

  {
    optional<bool> boolean_argument = false;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_boolean());
    EXPECT_EQ(payload.arguments[0].GetBoolean(), false);
    // The call shouldn't modify arguments passed by reference.
    EXPECT_TRUE(boolean_argument);
  }
}

TEST(ToRemoteCallRequestConversion, OptionalArgumentByConstReference) {
  {
    const optional<bool> boolean_argument = {};
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    EXPECT_TRUE(payload.arguments[0].is_null());
  }

  {
    const optional<bool> boolean_argument = false;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    ASSERT_EQ(payload.arguments.size(), 1U);
    ASSERT_TRUE(payload.arguments[0].is_boolean());
    EXPECT_EQ(payload.arguments[0].GetBoolean(), false);
  }
}

TEST(ToRemoteCallRequestConversion, MultipleArguments) {
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, -1, true, "");
  EXPECT_EQ(payload.function_name, kSomeFunc);
  ASSERT_EQ(payload.arguments.size(), 3U);
  ASSERT_TRUE(payload.arguments[0].is_integer());
  EXPECT_EQ(payload.arguments[0].GetInteger(), -1);
  ASSERT_TRUE(payload.arguments[1].is_boolean());
  EXPECT_EQ(payload.arguments[1].GetBoolean(), true);
  ASSERT_TRUE(payload.arguments[2].is_string());
  EXPECT_EQ(payload.arguments[2].GetString(), "");
}

TEST(ToRemoteCallRequestConversion, EnumArgument) {
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, SomeEnum::kFirst);
  EXPECT_EQ(payload.function_name, kSomeFunc);
  ASSERT_EQ(payload.arguments.size(), 1U);
  ASSERT_TRUE(payload.arguments[0].is_string());
  EXPECT_EQ(payload.arguments[0].GetString(), "first");
}

TEST(ToRemoteCallRequestConversion, StructArgument) {
  SomeStruct some_struct;
  some_struct.foo = 123;
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, some_struct);
  EXPECT_EQ(payload.function_name, kSomeFunc);
  ASSERT_EQ(payload.arguments.size(), 1U);
  ASSERT_TRUE(payload.arguments[0].is_dictionary());
  EXPECT_EQ(payload.arguments[0].GetDictionary().size(), 1U);
  const Value* const item_foo = payload.arguments[0].GetDictionaryItem("foo");
  ASSERT_TRUE(item_foo);
  ASSERT_TRUE(item_foo->is_integer());
  EXPECT_EQ(item_foo->GetInteger(), 123);
}

}  // namespace google_smart_card
