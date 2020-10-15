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

#include <google_smart_card_common/pp_var_utils/struct_converter.h>

#include <string>

#include <gtest/gtest.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/pp_var_utils/extraction.h>

namespace google_smart_card {

namespace {

struct TestStruct {
  int int_field;
  std::string string_field;
  optional<int> optional_field_1;
  optional<int> optional_field_2;
};

using TestStructConverter = StructConverter<TestStruct>;

}  // namespace

// static
template <>
constexpr const char* TestStructConverter::GetStructTypeName() {
  return "TestStruct";
}

// static
template <>
template <typename Callback>
void TestStructConverter::VisitFields(const TestStruct& value,
                                      Callback callback) {
  callback(&value.int_field, "int_field");
  callback(&value.string_field, "string_field");
  callback(&value.optional_field_1, "optional_field_1");
  callback(&value.optional_field_2, "optional_field_2");
}

TEST(PpVarUtilsStructConverterTest, SampleStructConversion) {
  std::string error_message;

  TestStruct value;
  value.int_field = 123;
  value.string_field = "foo";
  value.optional_field_1 = 456;

  const pp::Var var = TestStructConverter::ConvertToVar(value);
  pp::VarDictionary var_dict;
  ASSERT_TRUE(VarAs(var, &var_dict, &error_message));
  int converted_int_field;
  ASSERT_TRUE(GetVarDictValueAs(var_dict, "int_field", &converted_int_field,
                                &error_message));
  EXPECT_EQ(123, converted_int_field);
  std::string converted_string_field;
  ASSERT_TRUE(GetVarDictValueAs(var_dict, "string_field",
                                &converted_string_field, &error_message));
  EXPECT_EQ("foo", converted_string_field);
  int converted_optional_field_1;
  ASSERT_TRUE(GetVarDictValueAs(var_dict, "optional_field_1",
                                &converted_optional_field_1, &error_message));
  EXPECT_EQ(456, converted_optional_field_1);
  EXPECT_EQ(3, GetVarDictSize(var_dict));

  TestStruct back_converted_value;
  ASSERT_TRUE(TestStructConverter::ConvertFromVar(var, &back_converted_value,
                                                  &error_message));
  EXPECT_EQ(value.int_field, back_converted_value.int_field);
  EXPECT_EQ(value.string_field, back_converted_value.string_field);
  EXPECT_TRUE(back_converted_value.optional_field_1);
  EXPECT_FALSE(back_converted_value.optional_field_2);
}

}  // namespace google_smart_card
