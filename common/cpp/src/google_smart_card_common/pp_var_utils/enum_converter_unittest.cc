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

#include <gtest/gtest.h>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/pp_var_utils/enum_converter.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

namespace google_smart_card {

namespace {

enum class TestEnum {
  kTestValue1,
  kTestValue2,
};

using TestEnumStringConverter = EnumConverter<TestEnum, std::string>;

}  // namespace

template <>
constexpr const char* TestEnumStringConverter::GetEnumTypeName() {
  return "TestEnum";
}

template <>
template <typename Callback>
void TestEnumStringConverter::VisitCorrespondingPairs(Callback callback) {
  callback(TestEnum::kTestValue1, "test_value_1");
  callback(TestEnum::kTestValue2, "test_value_2");
}

TEST(EnumConverterTest, EnumToStringConversion) {
  EXPECT_EQ(
      "test_value_1",
      VarAs<std::string>(TestEnumStringConverter::ConvertToVar(
          TestEnum::kTestValue1)));
  EXPECT_EQ(
      "test_value_2",
      VarAs<std::string>(TestEnumStringConverter::ConvertToVar(
          TestEnum::kTestValue2)));
}

TEST(EnumConverterTest, EnumFromStringConversion) {
  TestEnum enum_value;
  std::string error_message;

  ASSERT_TRUE(TestEnumStringConverter::ConvertFromVar(
      pp::Var("test_value_1"), &enum_value, &error_message));
  EXPECT_EQ(TestEnum::kTestValue1, enum_value);

  ASSERT_TRUE(TestEnumStringConverter::ConvertFromVar(
      pp::Var("test_value_2"), &enum_value, &error_message));
  EXPECT_EQ(TestEnum::kTestValue2, enum_value);

  error_message.clear();
  EXPECT_FALSE(TestEnumStringConverter::ConvertFromVar(
      pp::Var("some_bad_value"), &enum_value, &error_message));
  EXPECT_NE("", error_message);

  error_message.clear();
  EXPECT_FALSE(TestEnumStringConverter::ConvertFromVar(
      pp::Var(123), &enum_value, &error_message));
  EXPECT_NE("", error_message);
}

}  // namespace google_smart_card
