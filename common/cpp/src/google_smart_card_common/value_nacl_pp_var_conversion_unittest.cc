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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ppapi/cpp/resource.h>
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

TEST(ValueNaclPpVarConversion, IntegerNon32BitValue) {
  const int64_t k40Bit = 1LL << 40;
  const pp::Var var = ConvertValueToPpVar(Value(k40Bit));
  ASSERT_TRUE(var.is_double());
  EXPECT_DOUBLE_EQ(var.AsDouble(), static_cast<double>(k40Bit));
}

TEST(ValueNaclPpVarConversion, Integer64BitMaxValue) {
  const int64_t k64BitMax = std::numeric_limits<int64_t>::max();
  const pp::Var var = ConvertValueToPpVar(Value(k64BitMax));
  ASSERT_TRUE(var.is_double());
  EXPECT_DOUBLE_EQ(var.AsDouble(), static_cast<double>(k64BitMax));
}

TEST(ValueNaclPpVarConversion, Integer64BitMinValue) {
  const int64_t k64BitMin = std::numeric_limits<int64_t>::min();
  const pp::Var var = ConvertValueToPpVar(Value(k64BitMin));
  ASSERT_TRUE(var.is_double());
  EXPECT_DOUBLE_EQ(var.AsDouble(), static_cast<double>(k64BitMin));
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
    std::map<std::string, Value> inner_items;
    inner_items["foo"] = Value();
    inner_items["bar"] = Value(123);
    std::map<std::string, Value> items;
    items["xyz"] = std::move(inner_items);
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
    std::vector<Value> inner_items;
    inner_items.push_back(Value());
    inner_items.push_back(Value(123));
    std::vector<Value> items;
    items.push_back(std::move(inner_items));
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

TEST(ValueNaclPpVarConversion, UndefinedPpVar) {
  std::string error_message;
  const optional<Value> value = ConvertPpVarToValue(pp::Var(), &error_message);
  EXPECT_TRUE(error_message.empty());
  ASSERT_TRUE(value);
  EXPECT_TRUE(value->is_null());
}

TEST(ValueNaclPpVarConversion, NullPpVar) {
  std::string error_message;
  const optional<Value> value =
      ConvertPpVarToValue(pp::Var(pp::Var::Null()), &error_message);
  EXPECT_TRUE(error_message.empty());
  ASSERT_TRUE(value);
  EXPECT_TRUE(value->is_null());
}

TEST(ValueNaclPpVarConversion, BooleanPpVar) {
  {
    constexpr bool kBoolean = false;
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(pp::Var(kBoolean), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_boolean());
    EXPECT_EQ(value->GetBoolean(), kBoolean);
  }

  {
    constexpr bool kBoolean = true;
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(pp::Var(kBoolean), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_boolean());
    EXPECT_EQ(value->GetBoolean(), kBoolean);
  }
}

TEST(ValueNaclPpVarConversion, IntegerPpVar) {
  constexpr int kInteger = 123;
  std::string error_message;
  const optional<Value> value =
      ConvertPpVarToValue(pp::Var(kInteger), &error_message);
  EXPECT_TRUE(error_message.empty());
  ASSERT_TRUE(value);
  ASSERT_TRUE(value->is_integer());
  EXPECT_EQ(value->GetInteger(), kInteger);
}

TEST(ValueNaclPpVarConversion, FloatPpVar) {
  constexpr double kFloat = 123.456;
  std::string error_message;
  const optional<Value> value =
      ConvertPpVarToValue(pp::Var(kFloat), &error_message);
  EXPECT_TRUE(error_message.empty());
  ASSERT_TRUE(value);
  ASSERT_TRUE(value->is_float());
  EXPECT_DOUBLE_EQ(value->GetFloat(), kFloat);
}

TEST(ValueNaclPpVarConversion, StringPpVar) {
  constexpr char kString[] = "foo";
  std::string error_message;
  const optional<Value> value =
      ConvertPpVarToValue(pp::Var(pp::Var(kString)), &error_message);
  EXPECT_TRUE(error_message.empty());
  ASSERT_TRUE(value);
  ASSERT_TRUE(value->is_string());
  EXPECT_EQ(value->GetString(), kString);
}

TEST(ValueNaclPpVarConversion, ResourcePpVar) {
  {
    const optional<Value> value = ConvertPpVarToValue(pp::Var(pp::Resource()));
    EXPECT_FALSE(value);
  }

  {
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(pp::Var(pp::Resource()), &error_message);
    EXPECT_EQ(error_message, "Error converting: unsupported type \"resource\"");
    EXPECT_FALSE(value);
  }
}

TEST(ValueNaclPpVarConversion, PpVarArrayBuffer) {
  {
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(pp::VarArrayBuffer(), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_binary());
    EXPECT_TRUE(value->GetBinary().empty());
  }

  {
    const std::vector<uint8_t> kBytes = {1, 2, 3};
    pp::VarArrayBuffer var_array_buffer(kBytes.size());
    std::memcpy(var_array_buffer.Map(), &kBytes.front(), kBytes.size());
    var_array_buffer.Unmap();

    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(var_array_buffer, &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_binary());
    EXPECT_EQ(value->GetBinary(), kBytes);
  }
}

TEST(ValueNaclPpVarConversion, PpVarDictionary) {
  {
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(pp::VarDictionary(), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_dictionary());
    EXPECT_TRUE(value->GetDictionary().empty());
  }

  {
    // The test data is: {"xyz": {"foo": null, "bar": 123}}.
    pp::VarDictionary inner_var_dict;
    ASSERT_TRUE(inner_var_dict.Set("foo", pp::Var(pp::Var::Null())));
    ASSERT_TRUE(inner_var_dict.Set("bar", pp::Var(123)));
    pp::VarDictionary var_dict;
    ASSERT_TRUE(var_dict.Set("xyz", inner_var_dict));

    std::string error_message;
    const optional<Value> value = ConvertPpVarToValue(var_dict, &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_dictionary());
    EXPECT_EQ(value->GetDictionary().size(), 1U);
    const auto xyz_iter = value->GetDictionary().find("xyz");
    ASSERT_NE(xyz_iter, value->GetDictionary().end());
    const Value& inner_value = *xyz_iter->second;
    ASSERT_TRUE(inner_value.is_dictionary());
    EXPECT_EQ(inner_value.GetDictionary().size(), 2U);
    const auto foo_iter = inner_value.GetDictionary().find("foo");
    ASSERT_NE(foo_iter, inner_value.GetDictionary().end());
    const Value& foo_item_value = *foo_iter->second;
    EXPECT_TRUE(foo_item_value.is_null());
    const auto bar_iter = inner_value.GetDictionary().find("bar");
    ASSERT_NE(bar_iter, inner_value.GetDictionary().end());
    const Value& bar_item_value = *bar_iter->second;
    ASSERT_TRUE(bar_item_value.is_integer());
    EXPECT_EQ(bar_item_value.GetInteger(), 123);
  }
}

TEST(ValueNaclPpVarConversion, PpVarDictionaryWithBadItem) {
  pp::VarDictionary inner_var_dict;  // {"someInnerKey": <resource>}
  ASSERT_TRUE(inner_var_dict.Set("someInnerKey", pp::Var(pp::Resource())));
  pp::VarDictionary var_dict;  // {"someKey": {"someInnerKey": <resource>}}
  ASSERT_TRUE(var_dict.Set("someKey", inner_var_dict));

  {
    const optional<Value> value = ConvertPpVarToValue(inner_var_dict);
    EXPECT_FALSE(value);
  }

  {
    const optional<Value> value = ConvertPpVarToValue(var_dict);
    EXPECT_FALSE(value);
  }

  {
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(inner_var_dict, &error_message);
    EXPECT_EQ(error_message,
              "Error converting dictionary item \"someInnerKey\": Error "
              "converting: unsupported type \"resource\"");
    EXPECT_FALSE(value);
  }

  {
    std::string error_message;
    const optional<Value> value = ConvertPpVarToValue(var_dict, &error_message);
    EXPECT_EQ(error_message,
              "Error converting dictionary item \"someKey\": Error converting "
              "dictionary item \"someInnerKey\": Error converting: unsupported "
              "type \"resource\"");
    EXPECT_FALSE(value);
  }
}

TEST(ValueNaclPpVarConversion, PpVarArray) {
  {
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(pp::VarArray(), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_array());
    EXPECT_TRUE(value->GetArray().empty());
  }

  {
    // The test data is: [[null, 123]].
    pp::VarArray inner_var_array;
    ASSERT_TRUE(inner_var_array.Set(0, pp::Var(pp::Var::Null())));
    ASSERT_TRUE(inner_var_array.Set(1, pp::Var(123)));
    pp::VarArray var_array;
    ASSERT_TRUE(var_array.Set(0, inner_var_array));

    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(var_array, &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_array());
    ASSERT_EQ(value->GetArray().size(), 1U);
    const Value& inner_value = *value->GetArray()[0];
    ASSERT_TRUE(inner_value.is_array());
    ASSERT_EQ(inner_value.GetArray().size(), 2U);
    EXPECT_TRUE(inner_value.GetArray()[0]->is_null());
    ASSERT_TRUE(inner_value.GetArray()[1]->is_integer());
    EXPECT_EQ(inner_value.GetArray()[1]->GetInteger(), 123);
  }
}

TEST(ValueNaclPpVarConversion, PpVarArrayWithBadItem) {
  pp::VarArray inner_var_array;  // [<resource>]
  ASSERT_TRUE(inner_var_array.Set(0, pp::Var(pp::Resource())));
  pp::VarArray var_array;  // [null, [<resource>]]
  ASSERT_TRUE(var_array.Set(0, pp::Var()));
  ASSERT_TRUE(var_array.Set(1, inner_var_array));

  {
    const optional<Value> value = ConvertPpVarToValue(inner_var_array);
    EXPECT_FALSE(value);
  }

  {
    const optional<Value> value = ConvertPpVarToValue(var_array);
    EXPECT_FALSE(value);
  }

  {
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(inner_var_array, &error_message);
    EXPECT_EQ(error_message,
              "Error converting array item #0: Error converting: unsupported "
              "type \"resource\"");
    EXPECT_FALSE(value);
  }

  {
    std::string error_message;
    const optional<Value> value =
        ConvertPpVarToValue(var_array, &error_message);
    EXPECT_EQ(error_message,
              "Error converting array item #1: Error converting array item #0: "
              "Error converting: unsupported type \"resource\"");
    EXPECT_FALSE(value);
  }
}

// Test that `ConvertPpVarToValueOrDie()` succeeds on supported inputs. As death
// tests aren't supported, we don't test failure scenarios.
TEST(ValueNaclPpVarConversion, PpVarOrDie) {
  {
    constexpr bool kBoolean = false;
    const Value value = ConvertPpVarToValueOrDie(pp::Var(kBoolean));
    ASSERT_TRUE(value.is_boolean());
    EXPECT_EQ(value.GetBoolean(), kBoolean);
  }

  {
    const int kInteger = 123;
    const Value value = ConvertPpVarToValueOrDie(pp::Var(kInteger));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kInteger);
  }

  {
    pp::VarDictionary var_dict;
    ASSERT_TRUE(var_dict.Set("foo", pp::Var(pp::Var::Null())));
    const Value value = ConvertPpVarToValueOrDie(var_dict);
    ASSERT_TRUE(value.is_dictionary());
    EXPECT_EQ(value.GetDictionary().size(), 1U);
    const Value* foo_value = value.GetDictionaryItem("foo");
    ASSERT_TRUE(foo_value);
    EXPECT_TRUE(foo_value->is_null());
  }
}

}  // namespace google_smart_card
