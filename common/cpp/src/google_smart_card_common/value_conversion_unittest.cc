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

#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>

using testing::MatchesRegex;

namespace google_smart_card {

TEST(ValueConversion, BoolToValue) {
  {
    const Value value = ConvertToValue(false);
    ASSERT_TRUE(value.is_boolean());
    EXPECT_EQ(value.GetBoolean(), false);
  }

  {
    const Value value = ConvertToValue(true);
    ASSERT_TRUE(value.is_boolean());
    EXPECT_EQ(value.GetBoolean(), true);
  }
}

TEST(ValueConversion, ValueToBool) {
  {
    std::string error_message;
    const optional<bool> converted =
        ConvertFromValue<bool>(Value(false), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(converted);
    EXPECT_EQ(*converted, false);
  }

  {
    std::string error_message;
    bool converted = true;
    EXPECT_TRUE(ConvertFromValue(Value(false), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, false);
  }

  {
    const optional<bool> converted = ConvertFromValue<bool>(Value(true));
    ASSERT_TRUE(converted);
    EXPECT_EQ(*converted, true);
  }

  {
    bool converted = false;
    EXPECT_TRUE(ConvertFromValue(Value(true), &converted));
    EXPECT_EQ(converted, true);
  }
}

TEST(ValueConversion, ValueToBoolError) {
  {
    std::string error_message;
    const optional<bool> converted =
        ConvertFromValue<bool>(Value(), &error_message);
    EXPECT_EQ(error_message,
              "Expected value of type boolean, instead got: null");
    EXPECT_FALSE(converted);
  }

  {
    std::string error_message;
    bool converted;
    EXPECT_FALSE(ConvertFromValue(Value(123), &converted, &error_message));
#ifdef NDEBUG
    EXPECT_EQ(error_message,
              "Expected value of type boolean, instead got: integer");
#else
    EXPECT_EQ(error_message,
              "Expected value of type boolean, instead got: 0x7B");
#endif
  }

  {
    const optional<bool> converted = ConvertFromValue<bool>(Value("false"));
    EXPECT_FALSE(converted);
  }

  {
    bool converted;
    EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));
  }
}

TEST(ValueConversion, IntToValue) {
  {
    constexpr int kInteger = 123;
    const Value value = ConvertToValue(kInteger);
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kInteger);
  }

  {
    constexpr int kIntMax = std::numeric_limits<int>::max();
    const Value value = ConvertToValue(kIntMax);
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kIntMax);
  }

  {
    constexpr int kIntMin = std::numeric_limits<int>::min();
    const Value value = ConvertToValue(kIntMin);
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kIntMin);
  }
}

TEST(ValueConversion, ValueToInt) {
  {
    constexpr int kInteger = 123;
    std::string error_message;
    const optional<int> converted =
        ConvertFromValue<int>(Value(kInteger), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(converted);
    EXPECT_EQ(*converted, kInteger);
  }

  {
    constexpr int kIntMax = std::numeric_limits<int>::max();
    std::string error_message;
    int converted = 0;
    EXPECT_TRUE(ConvertFromValue(Value(kIntMax), &converted, &error_message));
    EXPECT_TRUE(error_message.empty());
    EXPECT_EQ(converted, kIntMax);
  }

  {
    constexpr int kIntMin = std::numeric_limits<int>::min();
    const optional<int> converted = ConvertFromValue<int>(Value(kIntMin));
    ASSERT_TRUE(converted);
    EXPECT_EQ(*converted, kIntMin);
  }

  {
    constexpr int kInteger = 123;
    int converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<double>(kInteger)), &converted));
    EXPECT_EQ(converted, kInteger);
  }
}

// Test conversion of `kFloat` `Value` with an integer number into `int`.
TEST(ValueConversion, FloatValueToInt) {
  {
    constexpr int kInteger = 123;
    const optional<int> converted =
        ConvertFromValue<int>(Value(static_cast<double>(kInteger)));
    ASSERT_TRUE(converted);
    EXPECT_EQ(*converted, kInteger);
  }

  {
    constexpr int kIntMax = std::numeric_limits<int>::max();
    int converted = 0;
    EXPECT_TRUE(
        ConvertFromValue(Value(static_cast<double>(kIntMax)), &converted));
    EXPECT_EQ(converted, kIntMax);
  }
}

TEST(ValueConversion, ValueToIntError) {
  {
    std::string error_message;
    const optional<int> converted =
        ConvertFromValue<int>(Value(), &error_message);
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
    EXPECT_FALSE(converted);
  }

  {
    std::string error_message;
    int converted;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message,
                ::testing::StartsWith("The real value is outside the exact "
                                      "integer representation range"));
  }

  {
    const optional<int> converted = ConvertFromValue<int>(Value("123"));
    EXPECT_FALSE(converted);
  }

  {
    int converted;
    EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kDictionary), &converted));
  }

  // It's only possible to test the "too big/small integer value" scenarios in
  // case the `int` size is smaller than `int64_t` that is used by `Value`.
  // According to the C++ Standard, that's not guaranteed to be the case (even
  // though a typical implementation does have it of <=4 bytes size).
  if (sizeof(int) < sizeof(int64_t)) {
    {
      const int64_t kInt64Max = std::numeric_limits<int64_t>::max();
      std::string error_message;
      EXPECT_FALSE(ConvertFromValue<int>(Value(kInt64Max), &error_message));
      EXPECT_THAT(error_message,
                  MatchesRegex(
                      "The integer value is outside the range of type \"int\": "
                      "9223372036854775807 not in .* range"));
    }

    {
      const int64_t kInt64Min = std::numeric_limits<int64_t>::min();
      std::string error_message;
      EXPECT_FALSE(ConvertFromValue<int>(Value(kInt64Min), &error_message));
      EXPECT_THAT(error_message,
                  MatchesRegex(
                      "The integer value is outside the range of type \"int\": "
                      "-9223372036854775808 not in .* range"));
    }
  }
}

TEST(ValueConversion, UnsignedToValue) {
  constexpr unsigned kInteger = 32767;
  const Value value = ConvertToValue(kInteger);
  ASSERT_TRUE(value.is_integer());
  EXPECT_EQ(value.GetInteger(), kInteger);
}

TEST(ValueConversion, ValueToUnsigned) {
  constexpr unsigned kInteger = 32767;
  unsigned converted = 0;
  EXPECT_TRUE(
      ConvertFromValue(Value(static_cast<int64_t>(kInteger)), &converted));
  EXPECT_EQ(converted, kInteger);
}

TEST(ValueConversion, ValueToUnsignedError) {
  {
    std::string error_message;
    const optional<unsigned> converted =
        ConvertFromValue<unsigned>(Value(), &error_message);
    EXPECT_EQ(error_message,
              "Expected value of type integer, instead got: null");
    EXPECT_FALSE(converted);
  }

  {
    std::string error_message;
    unsigned converted;
    EXPECT_FALSE(ConvertFromValue(Value(1E100), &converted, &error_message));
    EXPECT_THAT(error_message,
                ::testing::StartsWith("The real value is outside the exact "
                                      "integer representation range"));
  }

  {
    const optional<unsigned> converted =
        ConvertFromValue<unsigned>(Value("123"));
    EXPECT_FALSE(converted);
  }

  {
    unsigned converted;
    EXPECT_FALSE(ConvertFromValue(Value(Value::Type::kArray), &converted));
  }

  {
    const int kNegative = -1;
    std::string error_message;
    EXPECT_FALSE(ConvertFromValue<unsigned>(Value(kNegative), &error_message));
    EXPECT_THAT(error_message,
                MatchesRegex("The integer value is outside the range of type "
                             "\"unsigned\": -1 not in .* range"));
  }

  // It's only possible to test the "too big integer value" scenario in
  // case the `unsigned` size is smaller than `int64_t` that is used by `Value`.
  // According to the C++ Standard, that's not guaranteed to be the case (even
  // though a typical implementation does have it of <=4 bytes size).
  if (sizeof(unsigned) < sizeof(int64_t)) {
    {
      const int64_t kInt64Max = std::numeric_limits<int64_t>::max();
      std::string error_message;
      EXPECT_FALSE(
          ConvertFromValue<unsigned>(Value(kInt64Max), &error_message));
      EXPECT_THAT(
          error_message,
          MatchesRegex("The integer value is outside the range of type "
                       "\"unsigned\": 9223372036854775807 not in .* range"));
    }
  }
}

}  // namespace google_smart_card
