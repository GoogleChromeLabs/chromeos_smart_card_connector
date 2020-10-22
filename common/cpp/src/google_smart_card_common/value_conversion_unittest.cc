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

#include <google_smart_card_common/value_conversion.h>

#include <stdint.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <google_smart_card_common/value.h>

using testing::MatchesRegex;
using testing::StartsWith;

namespace google_smart_card {

enum class SomeEnum { kFirst, kSecond = 222, kSomeThird = 3, kForgotten = 456 };

template <>
EnumValueDescriptor<SomeEnum>::Description
EnumValueDescriptor<SomeEnum>::GetDescription() {
  return Describe("SomeEnum")
      .WithItem(SomeEnum::kFirst, "first")
      .WithItem(SomeEnum::kSecond, "second")
      .WithItem(SomeEnum::kSomeThird, "someThird");
}

TEST(ValueConversion, ValueToValue) {
  {
    std::string error_message;
    Value converted;
    EXPECT_TRUE(ConvertToValue(Value(123), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(converted.is_integer());
    EXPECT_EQ(converted.GetInteger(), 123);
  }

  {
    Value converted;
    EXPECT_TRUE(ConvertToValue(Value(), &converted));
    EXPECT_TRUE(converted.is_null());
  }

  {
    std::string error_message;
    Value converted;
    EXPECT_TRUE(ConvertFromValue(Value("foo"), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(converted.is_string());
    EXPECT_EQ(converted.GetString(), "foo");
  }

  {
    Value converted;
    EXPECT_TRUE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));
    ASSERT_TRUE(converted.is_dictionary());
    EXPECT_TRUE(converted.GetDictionary().empty());
  }
}

TEST(ValueConversion, BoolToValue) {
  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(false, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_boolean());
    EXPECT_EQ(value.GetBoolean(), false);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(true, &value));
    ASSERT_TRUE(value.is_boolean());
    EXPECT_EQ(value.GetBoolean(), true);
  }
}

TEST(ValueConversion, ValueToBool) {
  {
    std::string error_message;
    bool converted = true;
    EXPECT_TRUE(ConvertFromValue(Value(false), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, false);
  }

  {
    bool converted = false;
    EXPECT_TRUE(ConvertFromValue(Value(true), &converted));
    EXPECT_EQ(converted, true);
  }
}

TEST(ValueConversion, ValueToBoolError) {
  bool converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type boolean, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(123), &converted, &error_message));
#ifdef NDEBUG
    EXPECT_EQ(error_message,
              "Expected value of type boolean, instead got: integer");
#else
    EXPECT_EQ(error_message,
              "Expected value of type boolean, instead got: 0x7B");
#endif
  }

  EXPECT_FALSE(ConvertFromValue(Value("false"), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));
}

TEST(ValueConversion, IntToValue) {
  constexpr int kNumber = 123;
  constexpr int kIntMax = std::numeric_limits<int>::max();
  constexpr int kIntMin = std::numeric_limits<int>::min();

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kNumber, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kNumber);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kIntMax, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kIntMax);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kIntMin, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kIntMin);
  }
}

TEST(ValueConversion, ValueToInt) {
  constexpr int kNumber = 123;
  constexpr int kIntMax = std::numeric_limits<int>::max();
  constexpr int kIntMin = std::numeric_limits<int>::min();

  {
    std::string error_message;
    int converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kNumber), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    int converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kIntMax), &converted));
    EXPECT_EQ(converted, kIntMax);
  }

  {
    int converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kIntMin), &converted));
    EXPECT_EQ(converted, kIntMin);
  }
}

// Test conversion of `kFloat` `Value` with an integer number into `int`.
TEST(ValueConversion, FloatValueToInt) {
  constexpr int kNumber = 123;
  // This is a big number that is guaranteed to fit into `int` and into the
  // range of precisely representable `double` types. (The Standard doesn't give
  // specific lower boundary for the latter, but our rought approximation below
  // is based on the specified minimum precision requirement.)
  constexpr int kBig =
      (sizeof(int) < 4) ? std::numeric_limits<int>::max() : (1 << 30);
  constexpr int kBigNegative =
      (sizeof(int) < 4) ? std::numeric_limits<int>::min() : -(1 << 30);

  {
    std::string error_message;
    int converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kNumber)),
                                 &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    int converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kBig)), &converted));
    EXPECT_EQ(converted, kBig);
  }

  {
    int converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<double>(kBigNegative)), &converted));
    EXPECT_EQ(converted, kBigNegative);
  }
}

TEST(ValueConversion, ValueToIntError) {
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  constexpr int64_t kInt64Min = std::numeric_limits<int64_t>::min();
  int converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message, StartsWith("The real value is outside the exact "
                                          "integer representation range"));
  }

  EXPECT_FALSE(ConvertFromValue(Value("123"), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));

  // It's only possible to test the "too big/small integer value" scenarios in
  // case the `int` size is smaller than `int64_t` that is used by `Value`.
  // According to the C++ Standard, that's not guaranteed to be the case (even
  // though a typical implementation does have it of <=4 bytes size).
  if (sizeof(int) < sizeof(int64_t)) {
    {
      std::string error_message;
      EXPECT_FALSE(
          ConvertFromValue(Value(kInt64Max), &converted, &error_message));
      EXPECT_THAT(error_message,
                  MatchesRegex(
                      "The integer value is outside the range of type \"int\": "
                      "9223372036854775807 not in .* range"));
    }

    {
      std::string error_message;
      EXPECT_FALSE(
          ConvertFromValue(Value(kInt64Min), &converted, &error_message));
      EXPECT_THAT(error_message,
                  MatchesRegex(
                      "The integer value is outside the range of type \"int\": "
                      "-9223372036854775808 not in .* range"));
    }
  }
}

TEST(ValueConversion, UnsignedToValue) {
  constexpr unsigned kZero = 0;
  // This is the maximum number that is guaranteed to fit into `unsigned`
  // regardless of its size.
  constexpr unsigned kNumber = (1LL << 16) - 1;

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kZero, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kZero);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kNumber, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kNumber);
  }
}

TEST(ValueConversion, ValueToUnsigned) {
  constexpr unsigned kZero = 0;
  // This is the maximum number that is guaranteed to fit into `unsigned`
  // regardless of its size.
  constexpr unsigned kNumber = (1LL << 16) - 1;

  {
    std::string error_message;
    unsigned converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<int64_t>(kZero)), &converted,
                                 &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kZero);
  }

  {
    unsigned converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<int64_t>(kNumber)), &converted));
    EXPECT_EQ(converted, kNumber);
  }
}

// Test conversion of `kFloat` `Value` with an integer number into `unsigned`.
TEST(ValueConversion, FloatValueToUnsigned) {
  constexpr unsigned kNumber = 123;
  // This is a big number that is guaranteed to fit into `unsigned` and into the
  // range of precisely representable `double` types. (The Standard doesn't give
  // specific lower boundary for the latter, but our rought approximation below
  // is based on the specified minimum precision requirement.)
  constexpr unsigned kBig =
      (sizeof(unsigned) < 4) ? std::numeric_limits<unsigned>::max() : (1 << 30);

  {
    std::string error_message;
    unsigned converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kNumber)),
                                 &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    unsigned converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kBig)), &converted));
    EXPECT_EQ(converted, kBig);
  }
}

TEST(ValueConversion, ValueToUnsignedError) {
  constexpr int kNegative = -1;
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  unsigned converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message, StartsWith("The real value is outside the exact "
                                          "integer representation range"));
  }

  EXPECT_FALSE(ConvertFromValue(Value("123"), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kArray), &converted));

  {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value(kNegative), &converted, &error_message));
    EXPECT_THAT(error_message,
                MatchesRegex("The integer value is outside the range of type "
                             "\"unsigned\": -1 not in .* range"));
  }

  // It's only possible to test the "too big integer value" scenario in
  // case the `unsigned` size is smaller than `int64_t` that is used by `Value`.
  // According to the C++ Standard, that's not guaranteed to be the case (even
  // though a typical implementation does have it of <=4 bytes size).
  if (sizeof(unsigned) < sizeof(int64_t)) {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value(kInt64Max), &converted, &error_message));
    EXPECT_THAT(
        error_message,
        MatchesRegex("The integer value is outside the range of type "
                     "\"unsigned\": 9223372036854775807 not in .* range"));
  }
}

TEST(ValueConversion, LongToValue) {
  constexpr long kNumber = 123;
  constexpr long kLongMax = std::numeric_limits<long>::max();
  constexpr long kLongMin = std::numeric_limits<long>::min();

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kNumber, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kNumber);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kLongMax, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kLongMax);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kLongMin, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kLongMin);
  }
}

TEST(ValueConversion, ValueToLong) {
  constexpr long kNumber = 123;
  constexpr long kLongMax = std::numeric_limits<long>::max();
  constexpr long kLongMin = std::numeric_limits<long>::min();

  {
    std::string error_message;
    long converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<int64_t>(kNumber)),
                                 &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    long converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<int64_t>(kLongMax)), &converted));
    EXPECT_EQ(converted, kLongMax);
  }

  {
    long converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<int64_t>(kLongMin)), &converted));
    EXPECT_EQ(converted, kLongMin);
  }
}

// Test conversion of `kFloat` `Value` with an integer number into `long`.
TEST(ValueConversion, FloatValueToLong) {
  constexpr long kNumber = 123;
  // This is a big number that is guaranteed to fit into `int` and into the
  // range of precisely representable `double` types. (The Standard doesn't give
  // specific lower boundary for the latter, but our rought approximation below
  // is based on the specified minimum precision requirement.)
  constexpr long kBig =
      (sizeof(long) < 4) ? std::numeric_limits<long>::max() : (1 << 30);
  constexpr long kBigNegative =
      (sizeof(long) < 4) ? std::numeric_limits<long>::min() : -(1 << 30);

  {
    std::string error_message;
    long converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kNumber)),
                                 &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    long converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kBig)), &converted));
    EXPECT_EQ(converted, kBig);
  }

  {
    long converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<double>(kBigNegative)), &converted));
    EXPECT_EQ(converted, kBigNegative);
  }
}

TEST(ValueConversion, ValueToLongError) {
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  constexpr int64_t kInt64Min = std::numeric_limits<int64_t>::min();
  long converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message, StartsWith("The real value is outside the exact "
                                          "integer representation range"));
  }

  EXPECT_FALSE(ConvertFromValue(Value("123"), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));

  // It's only possible to test the "too big/small integer value" scenarios in
  // case the `long` size is smaller than `int64_t` that is used by `Value`.
  if (sizeof(long) < sizeof(int64_t)) {
    {
      std::string error_message;
      EXPECT_FALSE(
          ConvertFromValue(Value(kInt64Max), &converted, &error_message));
      EXPECT_THAT(
          error_message,
          MatchesRegex("The integer value is outside the range of type "
                       "\"long\": 9223372036854775807 not in .* range"));
    }

    {
      std::string error_message;
      EXPECT_FALSE(
          ConvertFromValue(Value(kInt64Min), &converted, &error_message));
      EXPECT_THAT(
          error_message,
          MatchesRegex("The integer value is outside the range of type "
                       "\"long\": -9223372036854775808 not in .* range"));
    }
  }
}

TEST(ValueConversion, UnsignedLongToValue) {
  constexpr unsigned long kZero = 0;
  // This is the maximum number that is guaranteed to fit into `unsigned long`
  // regardless of its size.
  constexpr unsigned long kNumber = (1LL << 32) - 1;

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kZero, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kZero);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kNumber, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kNumber);
  }
}

TEST(ValueConversion, ValueToUnsignedLong) {
  constexpr int64_t kZero = 0;
  // This is the maximum number that is guaranteed to fit into `unsigned long`
  // regardless of its size.
  constexpr int64_t kNumber = (1LL << 32) - 1;

  {
    std::string error_message;
    unsigned long converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kZero), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kZero);
  }

  {
    unsigned long converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kNumber), &converted));
    EXPECT_EQ(converted, kNumber);
  }
}

// Test conversion of `kFloat` `Value` with an integer number into
// `unsigned long`.
TEST(ValueConversion, FloatValueToUnsignedLong) {
  constexpr int64_t kNumber = 123;
  // This is a big number that is guaranteed to fit into `unsigned` and into the
  // range of precisely representable `double` types. (The Standard doesn't give
  // specific lower boundary for the latter, but our rought approximation below
  // is based on the specified minimum precision requirement.)
  constexpr int64_t kBig = (sizeof(unsigned long) < 4)
                               ? std::numeric_limits<unsigned long>::max()
                               : (1LL << 30);

  {
    std::string error_message;
    unsigned long converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kNumber)),
                                 &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    unsigned long converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kBig)), &converted));
    EXPECT_EQ(converted, kBig);
  }
}

TEST(ValueConversion, ValueToUnsignedLongError) {
  constexpr int kNegative = -1;
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  unsigned long converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message, StartsWith("The real value is outside the exact "
                                          "integer representation range"));
  }

  EXPECT_FALSE(ConvertFromValue(Value("123"), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kArray), &converted));

  {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value(kNegative), &converted, &error_message));
    EXPECT_THAT(error_message,
                MatchesRegex("The integer value is outside the range of type "
                             "\"unsigned long\": -1 not in .* range"));
  }

  // It's only possible to test the "too big integer value" scenario in
  // case the `unsigned` size is smaller than `int64_t` that is used by `Value`.
  if (sizeof(unsigned long) < sizeof(int64_t)) {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value(kInt64Max), &converted, &error_message));
    EXPECT_THAT(
        error_message,
        MatchesRegex("The integer value is outside the range of type "
                     "\"unsigned long\": 9223372036854775807 not in .* range"));
  }
}

TEST(ValueConversion, Uint8ToValue) {
  constexpr uint8_t kZero = 0;
  constexpr uint8_t kUint8Max = std::numeric_limits<uint8_t>::max();

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kZero, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kZero);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kUint8Max, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kUint8Max);
  }
}

TEST(ValueConversion, ValueToUint8) {
  constexpr uint8_t kZero = 0;
  constexpr uint8_t kUint8Max = std::numeric_limits<uint8_t>::max();

  {
    std::string error_message;
    uint8_t converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<int64_t>(kZero)), &converted,
                                 &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kZero);
  }

  {
    uint8_t converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<int64_t>(kUint8Max)), &converted));
    EXPECT_EQ(converted, kUint8Max);
  }
}

// Test conversion of `kFloat` `Value` with an integer number into `uint8_t`.
TEST(ValueConversion, FloatValueToUint8) {
  constexpr uint8_t kNumber = 123;
  constexpr uint8_t kUint8Max = std::numeric_limits<uint8_t>::max();

  {
    std::string error_message;
    uint8_t converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kNumber)),
                                 &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    uint8_t converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<double>(kUint8Max)), &converted));
    EXPECT_EQ(converted, kUint8Max);
  }
}

TEST(ValueConversion, ValueToUint8Error) {
  constexpr int kNegative = -1;
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  uint8_t converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message, StartsWith("The real value is outside the exact "
                                          "integer representation range"));
  }

  EXPECT_FALSE(ConvertFromValue(Value("123"), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kArray), &converted));

  {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value(kNegative), &converted, &error_message));
    EXPECT_THAT(error_message,
                MatchesRegex("The integer value is outside the range of type "
                             "\"uint8_t\": -1 not in .* range"));
  }

  {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value(kInt64Max), &converted, &error_message));
    EXPECT_THAT(
        error_message,
        MatchesRegex("The integer value is outside the range of type "
                     "\"uint8_t\": 9223372036854775807 not in .* range"));
  }
}

TEST(ValueConversion, Int64ToValue) {
  constexpr int64_t kNumber = 123;
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  constexpr int64_t kInt64Min = std::numeric_limits<int64_t>::min();

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kNumber, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kNumber);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kInt64Max, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kInt64Max);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kInt64Min, &value));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kInt64Min);
  }
}

TEST(ValueConversion, ValueToInt64) {
  constexpr int64_t kNumber = 123;
  constexpr int64_t kIntMax = std::numeric_limits<int64_t>::max();
  constexpr int64_t kIntMin = std::numeric_limits<int64_t>::min();

  {
    std::string error_message;
    int64_t converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kNumber), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    int64_t converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kIntMax), &converted));
    EXPECT_EQ(converted, kIntMax);
  }

  {
    int64_t converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kIntMin), &converted));
    EXPECT_EQ(converted, kIntMin);
  }
}

// Test conversion of `kFloat` `Value` with an integer number into `int64_t`.
TEST(ValueConversion, FloatValueToInt64) {
  constexpr int64_t kNumber = 123;
  // This is a big number that is guaranteed to fit into the range of precisely
  // representable `double` types. (The Standard doesn't give specific lower
  // boundary for the latter, but our rought approximation below is based on the
  // specified minimum precision requirement.)
  constexpr int64_t kBig = 1LL << 30;
  constexpr int64_t kBigNegative = -(1LL << 30);

  {
    std::string error_message;
    int64_t converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kNumber)),
                                 &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kNumber);
  }

  {
    int64_t converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(static_cast<double>(kBig)), &converted));
    EXPECT_EQ(converted, kBig);
  }

  {
    int64_t converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<double>(kBigNegative)), &converted));
    EXPECT_EQ(converted, kBigNegative);
  }
}

TEST(ValueConversion, ValueToInt64Error) {
  int64_t converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message, StartsWith("The real value is outside the exact "
                                          "integer representation range"));
  }

  EXPECT_FALSE(ConvertFromValue(Value("123"), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));
}

TEST(ValueConversion, DoubleToValue) {
  constexpr double kFractional = 123.456;
  constexpr double kHuge = 1E100;

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kFractional, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_float());
    EXPECT_DOUBLE_EQ(value.GetFloat(), kFractional);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kHuge, &value));
    ASSERT_TRUE(value.is_float());
    EXPECT_DOUBLE_EQ(value.GetFloat(), kHuge);
  }
}

TEST(ValueConversion, ValueToDouble) {
  constexpr double kFractional = 123.456;
  constexpr int kInteger = 123;
  constexpr double kDoubleMax = std::numeric_limits<double>::max();
  constexpr double kDoubleLowest = std::numeric_limits<double>::lowest();

  {
    std::string error_message;
    double converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(kFractional), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_DOUBLE_EQ(converted, kFractional);
  }

  {
    double converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kInteger), &converted));
    EXPECT_DOUBLE_EQ(converted, kInteger);
  }

  {
    double converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kDoubleMax), &converted));
    EXPECT_DOUBLE_EQ(converted, kDoubleMax);
  }

  {
    double converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kDoubleLowest), &converted));
    EXPECT_DOUBLE_EQ(converted, kDoubleLowest);
  }
}

TEST(ValueConversion, ValueToDoubleError) {
  constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
  double converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type integer or float, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_THAT(error_message,
                "Expected value of type integer or float, instead got: null");
  }

  EXPECT_FALSE(ConvertFromValue(Value(false), &converted));

  EXPECT_FALSE(ConvertFromValue(Value("123"), &converted));

  // Test the "too big integer" case, but only in case it's actually not fitting
  // within the double's exactly representable numbers range. The increment is
  // accounting the double's separate sign bit.
  if (std::numeric_limits<double>::digits + 1 < 64 &&
      std::numeric_limits<double>::radix == 2) {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value(kInt64Max), &converted, &error_message));
    EXPECT_THAT(
        error_message,
        MatchesRegex("The integer 9223372036854775807 cannot be converted into "
                     "a floating-point double value without loss of precision: "
                     "it is outside .* range"));
  }
}

TEST(ValueConversion, CharPointerToValue) {
  constexpr char kEmpty[] = "";
  constexpr char kFoo[] = "foo";

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kEmpty, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), kEmpty);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kFoo, &value));
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), kFoo);
  }
}

TEST(ValueConversion, StringToValue) {
  const std::string kEmpty;
  const std::string kFoo = "foo";

  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(kEmpty, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), kEmpty);
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(kFoo, &value));
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), kFoo);
  }
}

TEST(ValueConversion, ValueToString) {
  constexpr char kEmpty[] = "";
  constexpr char kFoo[] = "foo";

  {
    std::string error_message;
    std::string converted;
    EXPECT_TRUE(ConvertFromValue(Value(kEmpty), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kEmpty);
  }

  {
    std::string converted;
    EXPECT_TRUE(ConvertFromValue(Value(kFoo), &converted));
    EXPECT_EQ(converted, kFoo);
  }
}

TEST(ValueConversion, ValueToStringError) {
  std::string converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(error_message,
              "Expected value of type string, instead got: null");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_THAT(error_message,
                "Expected value of type string, instead got: null");
  }

  EXPECT_FALSE(ConvertFromValue(Value(false), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(123), &converted));
}

TEST(ValueConversion, EnumToValue) {
  {
    std::string error_message;
    Value value;
    EXPECT_TRUE(ConvertToValue(SomeEnum::kFirst, &value, &error_message));
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), "first");
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(SomeEnum::kSecond, &value));
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), "second");
  }

  {
    Value value;
    EXPECT_TRUE(ConvertToValue(SomeEnum::kSomeThird, &value));
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), "someThird");
  }
}

TEST(ValueConversion, EnumToValueError) {
  Value value;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertToValue(SomeEnum::kForgotten, &value, &error_message));
    EXPECT_EQ(
        error_message,
        "Cannot convert enum SomeEnum to value: unknown integer value 456");
  }

  EXPECT_FALSE(ConvertToValue(SomeEnum::kForgotten, &value));
}

TEST(ValueConversion, ValueToEnum) {
  {
    std::string error_message;
    SomeEnum converted = SomeEnum::kSecond;
    EXPECT_TRUE(ConvertFromValue(Value("first"), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, SomeEnum::kFirst);
  }

  {
    SomeEnum converted = SomeEnum::kFirst;
    EXPECT_TRUE(ConvertFromValue(Value("second"), &converted));
    EXPECT_EQ(converted, SomeEnum::kSecond);
  }

  {
    SomeEnum converted = SomeEnum::kFirst;
    EXPECT_TRUE(ConvertFromValue(Value("someThird"), &converted));
    EXPECT_EQ(converted, SomeEnum::kSomeThird);
  }
}

TEST(ValueConversion, ValueToEnumError) {
  SomeEnum converted;

  {
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue(Value(), &converted, &error_message));
    EXPECT_EQ(
        error_message,
        "Cannot convert value null to enum SomeEnum: value is not a string");
  }

  {
    std::string error_message;
    EXPECT_FALSE(
        ConvertFromValue(Value("nonExisting"), &converted, &error_message));
#ifdef NDEBUG
    EXPECT_EQ(
        error_message,
        "Cannot convert value string to enum SomeEnum: unknown enum value");
#else
    EXPECT_EQ(error_message,
              "Cannot convert value \"nonExisting\" to enum SomeEnum: unknown "
              "enum value");
#endif
  }

  EXPECT_FALSE(ConvertFromValue(Value(0), &converted));

  EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));
}

// Test that `ConvertToValueOrDie()` succeeds on supported inputs. As death
// tests aren't supported, we don't test failure scenarios.
TEST(ValueConversion, ToValueOrDie) {
  {
    const Value value = ConvertToValueOrDie(false);
    ASSERT_TRUE(value.is_boolean());
    EXPECT_EQ(value.GetBoolean(), false);
  }

  {
    const Value value = ConvertToValueOrDie(123);
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), 123);
  }

  {
    const Value value = ConvertToValueOrDie(SomeEnum::kFirst);
    ASSERT_TRUE(value.is_string());
    EXPECT_EQ(value.GetString(), "first");
  }
}

// Test that `ConvertFromValueOrDie()` succeeds on supported inputs. As death
// tests aren't supported, we don't test failure scenarios.
TEST(ValueConversion, FromValueOrDie) {
  {
    const bool converted = ConvertFromValueOrDie<bool>(Value(true));
    EXPECT_EQ(converted, true);
  }

  {
    const int converted = ConvertFromValueOrDie<int>(Value(-1));
    EXPECT_EQ(converted, -1);
  }

  {
    const SomeEnum converted = ConvertFromValueOrDie<SomeEnum>(Value("second"));
    EXPECT_EQ(converted, SomeEnum::kSecond);
  }
}

}  // namespace google_smart_card
