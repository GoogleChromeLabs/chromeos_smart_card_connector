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

#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

#include <stdint.h>

#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

TEST(ValueNaclPpVarConversion, NullValue) {
  EXPECT_TRUE(ConvertValueToPpVar(Value()).is_null());
}

TEST(ValueNaclPpVarConversion, BooleanValue) {
  {
    const bool kBoolean = false;
    const pp::Var var = ConvertValueToPpVar(Value(kBoolean));
    ASSERT_TRUE(var.is_bool());
    EXPECT_EQ(var.AsBool(), kBoolean);
  }

  {
    const bool kBoolean = true;
    const pp::Var var = ConvertValueToPpVar(Value(kBoolean));
    ASSERT_TRUE(var.is_bool());
    EXPECT_EQ(var.AsBool(), kBoolean);
  }
}

TEST(ValueNaclPpVarConversion, IntegerValue) {
  const int kInteger = 123;
  const pp::Var var = ConvertValueToPpVar(Value(kInteger));
  ASSERT_TRUE(var.is_int());
  EXPECT_EQ(var.AsInt(), kInteger);
}

TEST(ValueNaclPpVarConversion, Integer64BitMaxValue) {
  const int64_t k64BitMax = std::numeric_limits<int64_t>::max();
  const pp::Var var = ConvertValueToPpVar(Value(k64BitMax));
  ASSERT_TRUE(var.is_double());
  EXPECT_DOUBLE_EQ(var.AsDouble(), k64BitMax);
}

TEST(ValueNaclPpVarConversion, Integer64BitMinValue) {
  const int64_t k64BitMin = std::numeric_limits<int64_t>::min();
  const pp::Var var = ConvertValueToPpVar(Value(k64BitMin));
  ASSERT_TRUE(var.is_double());
  EXPECT_DOUBLE_EQ(var.AsDouble(), k64BitMin);
}

TEST(ValueNaclPpVarConversion, FloatValue) {
  const double kFloat = 123.456;
  const pp::Var var = ConvertValueToPpVar(Value(kFloat));
  ASSERT_TRUE(var.is_double());
  EXPECT_DOUBLE_EQ(var.AsDouble(), kFloat);
}

TEST(ValueNaclPpVarConversion, StringValue) {
  {
    const pp::Var var = ConvertValueToPpVar(Value(Value::Type::kString));
    ASSERT_TRUE(var.is_string());
    EXPECT_EQ(var.AsString(), "");
  }

  {
    const char kString[] = "foo";
    const pp::Var var = ConvertValueToPpVar(Value(kString));
    ASSERT_TRUE(var.is_string());
    EXPECT_EQ(var.AsString(), kString);
  }
}

TEST(ValueNaclPpVarConversion, BinaryValue) {
  {
    const pp::Var var = ConvertValueToPpVar(Value(Value::Type::kBinary));
    ASSERT_TRUE(var.is_array_buffer());
    pp::VarArrayBuffer var_array_buffer(var);
    EXPECT_EQ(var_array_buffer.ByteLength(), 0U);
  }

  {
    const std::vector<uint8_t> kBinary = {1, 2, 3};
    const pp::Var var = ConvertValueToPpVar(Value(kBinary));
    ASSERT_TRUE(var.is_array_buffer());
    pp::VarArrayBuffer var_array_buffer(var);
    EXPECT_EQ(var_array_buffer.ByteLength(), kBinary.size());
    const void* const buffer_contents = var_array_buffer.Map();
    EXPECT_EQ(std::memcmp(buffer_contents, &kBinary.front(), kBinary.size()),
              0);
    var_array_buffer.Unmap();
  }
}

TEST(ValueNaclPpVarConversion, DictionaryValue) {
  {
    const pp::Var var = ConvertValueToPpVar(Value(Value::Type::kDictionary));
    ASSERT_TRUE(var.is_dictionary());
    const pp::VarDictionary var_dict(var);
    EXPECT_EQ(var_dict.GetKeys().GetLength(), 0U);
  }

  {
    // The test data is: {"xyz": {"foo": null, "bar": 123}}.
    std::map<std::string, std::unique_ptr<Value>> inner_items;
    inner_items["foo"] = MakeUnique<Value>();
    inner_items["bar"] = MakeUnique<Value>(123);
    std::map<std::string, std::unique_ptr<Value>> items;
    items["xyz"] = MakeUnique<Value>(std::move(inner_items));
    const Value value(std::move(items));

    const pp::Var var = ConvertValueToPpVar(value);
    ASSERT_TRUE(var.is_dictionary());
    const pp::VarDictionary var_dict(var);
    EXPECT_EQ(var_dict.GetKeys().GetLength(), 1U);
    const pp::Var item_xyz = var_dict.Get("xyz");
    ASSERT_TRUE(item_xyz.is_dictionary());
    const pp::VarDictionary inner_dict(item_xyz);
    EXPECT_EQ(inner_dict.GetKeys().GetLength(), 2U);
    const pp::Var inner_item_foo = inner_dict.Get("foo");
    EXPECT_TRUE(inner_item_foo.is_null());
    const pp::Var inner_item_bar = inner_dict.Get("bar");
    ASSERT_TRUE(inner_item_bar.is_int());
    EXPECT_EQ(inner_item_bar.AsInt(), 123);
  }
}

TEST(ValueNaclPpVarConversion, ArrayValue) {
  {
    const pp::Var var = ConvertValueToPpVar(Value(Value::Type::kArray));
    ASSERT_TRUE(var.is_array());
    const pp::VarArray var_array(var);
    ASSERT_EQ(var_array.GetLength(), 0U);
  }

  {
    // The test data is: [[null, 123]].
    std::vector<std::unique_ptr<Value>> inner_items;
    inner_items.push_back(MakeUnique<Value>());
    inner_items.push_back(MakeUnique<Value>(123));
    std::vector<std::unique_ptr<Value>> items;
    items.push_back(MakeUnique<Value>(std::move(inner_items)));
    const Value value(std::move(items));

    const pp::Var var = ConvertValueToPpVar(value);
    ASSERT_TRUE(var.is_array());
    const pp::VarArray var_array(var);
    ASSERT_EQ(var_array.GetLength(), 1U);
    const pp::Var item0 = var_array.Get(0);
    ASSERT_TRUE(item0.is_array());
    const pp::VarArray inner_array(item0);
    ASSERT_EQ(inner_array.GetLength(), 2U);
    const pp::Var inner_item0 = inner_array.Get(0);
    EXPECT_TRUE(inner_item0.is_null());
    const pp::Var inner_item1 = inner_array.Get(1);
    ASSERT_TRUE(inner_item1.is_int());
    EXPECT_EQ(inner_item1.AsInt(), 123);
  }
}

}  // namespace google_smart_card
