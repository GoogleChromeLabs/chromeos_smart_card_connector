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

#include "common/cpp/src/public/requesting/remote_call_arguments_conversion.h"

#include <optional>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/cpp/src/public/value_test_utils.h"

using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;

namespace google_smart_card {

namespace {

constexpr char kSomeFunc[] = "someFunc";

enum class SomeEnum { kUnknown, kFirst };

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
  EXPECT_THAT(payload.arguments, IsEmpty());
}

TEST(ToRemoteCallRequestConversion, BasicArgumentByValue) {
  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, false);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals(false)));
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, 123);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals(123)));
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, "foo");
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals("foo")));
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, std::string("foo"));
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals("foo")));
  }

  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc,
                                               std::vector<int>({1, 10}));
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments,
                ElementsAre(StrictlyEquals(std::vector<int>({1, 10}))));
  }
}

TEST(ToRemoteCallRequestConversion, BasicArgumentByReference) {
  {
    bool boolean_value = false;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_value);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals(false)));
  }

  {
    std::string string_value = "foo";
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, string_value);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals("foo")));
    // The call shouldn't modify arguments passed by reference.
    EXPECT_EQ(string_value, "foo");
  }
}

TEST(ToRemoteCallRequestConversion, BasicArgumentByConstReference) {
  const bool kBooleanValue = false;
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, kBooleanValue);
  EXPECT_EQ(payload.function_name, kSomeFunc);
  EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals(kBooleanValue)));
}

TEST(ToRemoteCallRequestConversion, OptionalArgumentByValue) {
  {
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, std::optional<int>());
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments,
                ElementsAre(StrictlyEquals(Value::Type::kNull)));
  }

  {
    constexpr int kIntegerValue = 123;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(
            kSomeFunc, std::make_optional(kIntegerValue));
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals(kIntegerValue)));
  }
}

TEST(ToRemoteCallRequestConversion, OptionalArgumentByReference) {
  {
    std::optional<bool> boolean_argument;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments,
                ElementsAre(StrictlyEquals(Value::Type::kNull)));
  }

  {
    std::optional<bool> boolean_argument = false;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals(false)));
    // The call shouldn't modify arguments passed by reference.
    EXPECT_TRUE(boolean_argument);
  }
}

TEST(ToRemoteCallRequestConversion, OptionalArgumentByConstReference) {
  {
    const std::optional<bool> boolean_argument = {};
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments,
                ElementsAre(StrictlyEquals(Value::Type::kNull)));
  }

  {
    const std::optional<bool> boolean_argument = false;
    const RemoteCallRequestPayload payload =
        ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, boolean_argument);
    EXPECT_EQ(payload.function_name, kSomeFunc);
    EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals(false)));
  }
}

TEST(ToRemoteCallRequestConversion, MultipleArguments) {
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, -1, true, "");
  EXPECT_EQ(payload.function_name, kSomeFunc);
  EXPECT_THAT(payload.arguments,
              ElementsAre(StrictlyEquals(-1), StrictlyEquals(true),
                          StrictlyEquals("")));
}

TEST(ToRemoteCallRequestConversion, EnumArgument) {
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, SomeEnum::kFirst);
  EXPECT_EQ(payload.function_name, kSomeFunc);
  EXPECT_THAT(payload.arguments, ElementsAre(StrictlyEquals("first")));
}

TEST(ToRemoteCallRequestConversion, StructArgument) {
  SomeStruct some_struct;
  some_struct.foo = 123;
  const RemoteCallRequestPayload payload =
      ConvertToRemoteCallRequestPayloadOrDie(kSomeFunc, some_struct);
  EXPECT_EQ(payload.function_name, kSomeFunc);
  ASSERT_THAT(payload.arguments, SizeIs(1));
  EXPECT_THAT(payload.arguments[0], DictSizeIs(1));
  EXPECT_THAT(payload.arguments[0], DictContains("foo", 123));
}

TEST(RemoteCallArgumentsExtractor, NoArgumentsExpected) {
  {
    std::vector<Value> values;
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));
    EXPECT_TRUE(extractor.success());
    EXPECT_TRUE(extractor.Finish());
    EXPECT_TRUE(extractor.error_message().empty());
  }

  {
    RemoteCallArgumentsExtractor extractor(kSomeFunc,
                                           Value(Value::Type::kArray));
    extractor.Extract();
    EXPECT_TRUE(extractor.Finish());
  }
}

TEST(RemoteCallArgumentsExtractor, NoArgumentsExpectedError) {
  {
    std::vector<Value> values;
    values.emplace_back(123);
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));
    EXPECT_FALSE(extractor.Finish());
    EXPECT_FALSE(extractor.success());
#ifdef NDEBUG
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected exactly 0 "
              "arguments, received 1; first extra argument: integer");
#else
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected exactly 0 "
              "arguments, received 1; first extra argument: 0x7B");
#endif
  }

  {
    RemoteCallArgumentsExtractor extractor(kSomeFunc, Value());
    EXPECT_FALSE(extractor.Finish());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): Expected value of "
              "type array, instead got: null");
  }
}

TEST(RemoteCallArgumentsExtractor, BasicArgumentExpected) {
  {
    std::vector<Value> values;
    values.emplace_back(false);
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument = true;
    extractor.Extract(&bool_argument);
    EXPECT_TRUE(extractor.Finish());
    EXPECT_FALSE(bool_argument);
  }

  {
    Value value(Value::Type::kArray);
    value.GetArray().push_back(MakeUnique<Value>(123));
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(value));

    int int_argument = 0;
    extractor.Extract(&int_argument);
    EXPECT_TRUE(extractor.Finish());
    EXPECT_EQ(int_argument, 123);
  }

  {
    std::vector<Value> values;
    values.emplace_back("foo");
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    std::string string_argument;
    extractor.Extract(&string_argument);
    EXPECT_TRUE(extractor.Finish());
    EXPECT_EQ(string_argument, "foo");
  }
}

TEST(RemoteCallArgumentsExtractor, BasicArgumentExpectedError) {
  {
    std::vector<Value> values;
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument;
    extractor.Extract(&bool_argument);
    EXPECT_FALSE(extractor.success());
    EXPECT_FALSE(extractor.Finish());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected at least 1 "
              "argument(s), received only 0");
  }

  {
    std::vector<Value> values;
    values.emplace_back(false);
    values.emplace_back();
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    // Extraction of the argument from the first value succeeds.
    bool bool_argument = true;
    extractor.Extract(&bool_argument);
    EXPECT_TRUE(extractor.success());
    EXPECT_FALSE(bool_argument);

    // Finishing at this point triggers an error due to an extra value.
    EXPECT_FALSE(extractor.Finish());
    EXPECT_FALSE(extractor.success());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected exactly 1 "
              "arguments, received 2; first extra argument: null");
  }

  {
    std::vector<Value> values;
    values.emplace_back(123);
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument;
    extractor.Extract(&bool_argument);
    EXPECT_FALSE(extractor.Finish());
#ifdef NDEBUG
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert argument #0 for someFunc(): Expected value of "
              "type boolean, instead got: integer");
#else
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert argument #0 for someFunc(): Expected value of "
              "type boolean, instead got: 0x7B");
#endif
  }
}

TEST(RemoteCallArgumentsExtractor, MultipleArgumentsExpected) {
  {
    std::vector<Value> values;
    values.emplace_back(true);
    values.emplace_back("foo");
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument = false;
    std::string string_argument;
    extractor.Extract(&bool_argument, &string_argument);
    EXPECT_TRUE(extractor.Finish());
    EXPECT_TRUE(bool_argument);
    EXPECT_EQ(string_argument, "foo");
  }

  {
    std::vector<Value> values;
    values.emplace_back(true);
    values.emplace_back("foo");
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument = false;
    extractor.Extract(&bool_argument);
    EXPECT_TRUE(extractor.success());
    EXPECT_TRUE(bool_argument);

    std::string string_argument;
    extractor.Extract(&string_argument);
    EXPECT_TRUE(extractor.Finish());
    EXPECT_EQ(string_argument, "foo");
  }
}

TEST(RemoteCallArgumentsExtractor, MultipleArgumentsExpectedError) {
  {
    RemoteCallArgumentsExtractor extractor(kSomeFunc,
                                           Value(Value::Type::kArray));

    bool bool_argument;
    std::string string_argument;
    extractor.Extract(&bool_argument, &string_argument);
    EXPECT_FALSE(extractor.success());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected at least 2 "
              "argument(s), received only 0");
  }

  {
    std::vector<Value> values;
    values.emplace_back(true);
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument;
    std::string string_argument;
    extractor.Extract(&bool_argument, &string_argument);
    EXPECT_FALSE(extractor.success());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected at least 2 "
              "argument(s), received only 1");
  }

  {
    std::vector<Value> values;
    values.emplace_back(true);
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument = false;
    extractor.Extract(&bool_argument);
    EXPECT_TRUE(extractor.success());
    EXPECT_TRUE(bool_argument);

    std::string string_argument;
    extractor.Extract(&string_argument);
    EXPECT_FALSE(extractor.success());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected at least 2 "
              "argument(s), received only 1");
  }

  {
    std::vector<Value> values;
    values.emplace_back(true);
    values.emplace_back();
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument;
    std::string string_argument;
    extractor.Extract(&bool_argument, &string_argument);
    EXPECT_FALSE(extractor.success());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert argument #1 for someFunc(): Expected value of "
              "type string, instead got: null");
  }

  {
    std::vector<Value> values;
    values.emplace_back(true);
    values.emplace_back("foo");
    values.emplace_back();
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    bool bool_argument = false;
    std::string string_argument;
    extractor.Extract(&bool_argument, &string_argument);
    EXPECT_TRUE(extractor.success());
    EXPECT_TRUE(bool_argument);
    EXPECT_EQ(string_argument, "foo");

    EXPECT_FALSE(extractor.Finish());
    EXPECT_EQ(extractor.error_message(),
              "Failed to convert arguments for someFunc(): expected exactly 2 "
              "arguments, received 3; first extra argument: null");
  }
}

TEST(RemoteCallArgumentsExtractor, EnumArgumentExpected) {
  std::vector<Value> values;
  values.emplace_back("first");
  RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

  SomeEnum enum_argument = SomeEnum::kUnknown;
  extractor.Extract(&enum_argument);
  EXPECT_TRUE(extractor.Finish());
  EXPECT_EQ(enum_argument, SomeEnum::kFirst);
}

TEST(RemoteCallArgumentsExtractor, OptionalArgument) {
  {
    std::vector<Value> values;
    values.emplace_back(false);
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    std::optional<bool> bool_argument;
    extractor.Extract(&bool_argument);
    EXPECT_TRUE(extractor.Finish());
    ASSERT_TRUE(bool_argument);
    EXPECT_FALSE(*bool_argument);
  }

  {
    std::vector<Value> values;
    values.emplace_back();
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    std::optional<bool> bool_argument;
    extractor.Extract(&bool_argument);
    EXPECT_TRUE(extractor.Finish());
    EXPECT_FALSE(bool_argument);
  }

  {
    std::vector<Value> values;
    values.emplace_back("foo");
    values.emplace_back(123);
    RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));

    std::optional<std::string> string_argument;
    int int_argument = 0;
    extractor.Extract(&string_argument, &int_argument);
    EXPECT_TRUE(extractor.Finish());
    ASSERT_TRUE(string_argument);
    EXPECT_EQ(*string_argument, "foo");
    EXPECT_EQ(int_argument, 123);
  }
}

TEST(RemoteCallArgumentsExtractor, OptionalArgumentError) {
  std::vector<Value> values;
  values.emplace_back(false);
  RemoteCallArgumentsExtractor extractor(kSomeFunc, std::move(values));
  std::optional<std::string> string_argument;
  extractor.Extract(&string_argument);
  EXPECT_FALSE(extractor.success());
#ifdef NDEBUG
  EXPECT_EQ(extractor.error_message(),
            "Failed to convert argument #0 for someFunc(): Expected value of "
            "type string, instead got: boolean");
#else
  EXPECT_EQ(extractor.error_message(),
            "Failed to convert argument #0 for someFunc(): Expected value of "
            "type string, instead got: false");
#endif
}

TEST(ExtractRemoteCallArguments, Basic) {
  std::vector<Value> values;
  values.emplace_back(123);
  values.emplace_back("foo");

  std::string error_message;
  int int_argument = 0;
  std::string string_argument;
  EXPECT_TRUE(ExtractRemoteCallArguments(kSomeFunc, std::move(values),
                                         &error_message, &int_argument,
                                         &string_argument));
  EXPECT_TRUE(error_message.empty());
  EXPECT_EQ(int_argument, 123);
  EXPECT_EQ(string_argument, "foo");
}

TEST(ExtractRemoteCallArguments, Error) {
  std::string error_message;
  int int_argument;
  EXPECT_FALSE(ExtractRemoteCallArguments(kSomeFunc, std::vector<Value>(),
                                          &error_message, &int_argument));
  EXPECT_EQ(error_message,
            "Failed to convert arguments for someFunc(): expected at least 1 "
            "argument(s), received only 0");
}

// As death tests aren't supported, we don't test failure scenarios.
TEST(ExtractRemoteCallArgumentsOrDie, Basic) {
  std::vector<Value> values;
  values.emplace_back(123);
  values.emplace_back("foo");

  int int_argument = 0;
  std::string string_argument;
  ExtractRemoteCallArgumentsOrDie(kSomeFunc, std::move(values), &int_argument,
                                  &string_argument);
  EXPECT_EQ(int_argument, 123);
  EXPECT_EQ(string_argument, "foo");
}

}  // namespace google_smart_card
