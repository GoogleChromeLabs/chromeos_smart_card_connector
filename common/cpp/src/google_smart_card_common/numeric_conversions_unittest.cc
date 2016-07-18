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

#include <stdint.h>

#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <google_smart_card_common/numeric_conversions.h>

namespace google_smart_card {

class NumericConversionsDoubleCastingTest : public ::testing::Test {
 protected:
  static_assert(std::numeric_limits<double>::digits == 53,
                "The double type has unexpected size");

  template <typename NumericType>
  void TestValueInDoubleExactRange(NumericType value) const {
    std::string error_message;

    double double_value = 0;
    ASSERT_TRUE(CastIntegerToDouble(value, &double_value, &error_message)) <<
        "Failed to convert integer value " << value << " into double: " <<
        error_message;
    ASSERT_EQ(value, static_cast<NumericType>(double_value));

    int64_t int64_value = 0;
    ASSERT_TRUE(CastDoubleToInt64(
        double_value, &int64_value, &error_message)) << "Failed to convert " <<
        "double value " << double_value << " into 64-bit integer: " <<
        error_message;
    EXPECT_EQ(static_cast<int64_t>(value), int64_value);
  }

  template <typename NumericType>
  void TestValueOutsideDoubleExactRange(NumericType value) const {
    std::string error_message;

    double double_value = 0;
    EXPECT_FALSE(CastIntegerToDouble(value, &double_value, &error_message)) <<
        "Unexpectedly successful conversion from integer " << value <<
        " into double";

    double_value = static_cast<double>(value);
    int64_t int64_value = 0;
    EXPECT_FALSE(CastDoubleToInt64(
        double_value, &int64_value, &error_message)) << "Unexpectedly " <<
        "successful conversion from double value " << double_value << " into" <<
        " 64-bit integer";
  }
};

TEST_F(NumericConversionsDoubleCastingTest, ValuesInDoubleExactRange) {
  TestValueInDoubleExactRange(0);
  TestValueInDoubleExactRange(static_cast<uint64_t>(0));
  TestValueInDoubleExactRange(1);
  TestValueInDoubleExactRange(-1);
  TestValueInDoubleExactRange(1000);
  TestValueInDoubleExactRange(-1000);
  TestValueInDoubleExactRange(1LL << 32);
  TestValueInDoubleExactRange(-(1LL << 32));
  TestValueInDoubleExactRange(1LL << 52);
  TestValueInDoubleExactRange(-(1LL << 52));
  TestValueInDoubleExactRange((1LL << 53) - 1);
  TestValueInDoubleExactRange(-(1LL << 53) + 1);
  TestValueInDoubleExactRange(1LL << 53);
  TestValueInDoubleExactRange(-(1LL << 53));
}

TEST_F(NumericConversionsDoubleCastingTest, ValuesOutsideDoubleExactRange) {
  TestValueOutsideDoubleExactRange(1LL << 54);
  TestValueOutsideDoubleExactRange(-(1LL << 54));
  TestValueOutsideDoubleExactRange(std::numeric_limits<int64_t>::min());
  TestValueOutsideDoubleExactRange(std::numeric_limits<int64_t>::max());
  TestValueOutsideDoubleExactRange(std::numeric_limits<uint64_t>::max());
}

class NumericConversionsInt64ToIntegerCastingTest : public ::testing::Test {
 protected:
  template <typename TargetIntegerType>
  void TestCasting(
      int64_t value,
      const std::string& type_name,
      bool expected_success) const {
    TargetIntegerType result_value;
    std::string error_message;
    ASSERT_EQ(
        expected_success,
        CastInt64ToInteger(value, type_name, &result_value, &error_message)) <<
        "Conversion of " << value << " into " << type_name << " type " <<
        "finished unexpectedly: expected " <<
        (expected_success ? "successful" : "unsuccessful") << " conversion";
  }
};

TEST_F(NumericConversionsInt64ToIntegerCastingTest, Int64ToIntegerCasting) {
  TestCasting<int8_t>(0, "int8_t", true);
  TestCasting<int8_t>(127, "int8_t", true);
  TestCasting<int8_t>(128, "int8_t", false);
  TestCasting<int8_t>(-128, "int8_t", true);
  TestCasting<int8_t>(-129, "int8_t", false);

  TestCasting<uint8_t>(0, "uint8_t", true);
  TestCasting<uint8_t>(255, "uint8_t", true);
  TestCasting<uint8_t>(256, "uint8_t", false);
  TestCasting<uint8_t>(-1, "uint8_t", false);

  TestCasting<int64_t>(0, "int64_t", true);
  TestCasting<int64_t>(std::numeric_limits<int64_t>::max(), "int64_t", true);
  TestCasting<int64_t>(std::numeric_limits<int64_t>::min(), "int64_t", true);

  TestCasting<uint64_t>(0, "uint64_t", true);
  TestCasting<uint64_t>(std::numeric_limits<int64_t>::max(), "uint64_t", true);
  TestCasting<uint64_t>(-1, "uint64_t", false);
}

}  // namespace google_smart_card
