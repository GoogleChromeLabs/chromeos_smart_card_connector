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

#include "common/cpp/src/public/value_emscripten_val_conversion.h"

#include <stdint.h>

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <emscripten/val.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/cpp/src/public/value.h"

using testing::MatchesRegex;

namespace google_smart_card {

namespace {

int GetEmscriptenObjectValSize(const emscripten::val& val) {
  const emscripten::val keys =
      emscripten::val::global("Object").call<emscripten::val>("keys", val);
  return keys["length"].as<int>();
}

}  // namespace

TEST(ValueEmscriptenValConversion, NullValue) {
  const std::optional<emscripten::val> converted =
      ConvertValueToEmscriptenVal(Value());
  ASSERT_TRUE(converted);
  EXPECT_TRUE(converted->isNull());
}

TEST(ValueEmscriptenValConversion, BooleanValue) {
  {
    constexpr bool kBoolean = false;
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(kBoolean));
    ASSERT_TRUE(converted);
    ASSERT_EQ(converted->typeOf().as<std::string>(), "boolean");
    EXPECT_EQ(converted->as<bool>(), kBoolean);
  }

  {
    constexpr bool kBoolean = true;
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(kBoolean));
    ASSERT_TRUE(converted);
    ASSERT_EQ(converted->typeOf().as<std::string>(), "boolean");
    EXPECT_EQ(converted->as<bool>(), kBoolean);
  }
}

TEST(ValueEmscriptenValConversion, IntegerValue) {
  constexpr int kNumber = 123;
  const std::optional<emscripten::val> converted =
      ConvertValueToEmscriptenVal(Value(kNumber));
  ASSERT_TRUE(converted);
  ASSERT_TRUE(converted->isNumber());
  EXPECT_EQ(converted->as<int>(), kNumber);
}

TEST(ValueEmscriptenValConversion, IntegerNon32BitValue) {
  constexpr int64_t k40Bit = 1LL << 40;
  const std::optional<emscripten::val> converted =
      ConvertValueToEmscriptenVal(Value(k40Bit));
  ASSERT_TRUE(converted);
  ASSERT_TRUE(converted->isNumber());
  // `emscripten::val` doesn't provide a direct way to transform into a
  // non-32-bit integer, so compare string representations.
  EXPECT_EQ(converted->call<std::string>("toString"), std::to_string(k40Bit));
}

// Test that conversion of huge integers fails (as JavaScript numbers cannot
// store them precisely).
TEST(ValueEmscriptenValConversion, IntegerValueError) {
  {
    constexpr int64_t k64BitMax = std::numeric_limits<int64_t>::max();
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(k64BitMax));
    EXPECT_FALSE(converted);
  }

  {
    constexpr int64_t k64BitMin = std::numeric_limits<int64_t>::min();
    std::string error_message;
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(k64BitMin), &error_message);
    EXPECT_FALSE(converted);
    EXPECT_THAT(
        error_message,
        MatchesRegex("The integer -9223372036854775808 cannot be converted "
                     "into a floating-point double value without loss of "
                     "precision: it is outside .* range"));
  }
}

TEST(ValueEmscriptenValConversion, FloatValue) {
  constexpr double kFloat = 123.456;
  const std::optional<emscripten::val> converted =
      ConvertValueToEmscriptenVal(Value(kFloat));
  ASSERT_TRUE(converted);
  ASSERT_TRUE(converted->isNumber());
  EXPECT_DOUBLE_EQ(converted->as<double>(), static_cast<double>(kFloat));
}

TEST(ValueEmscriptenValConversion, StringValue) {
  {
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kString));
    ASSERT_TRUE(converted);
    ASSERT_TRUE(converted->isString());
    EXPECT_EQ(converted->as<std::string>(), "");
  }

  {
    const char kFoo[] = "foo";
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(kFoo));
    ASSERT_TRUE(converted);
    ASSERT_TRUE(converted->isString());
    EXPECT_EQ(converted->as<std::string>(), kFoo);
  }
}

TEST(ValueEmscriptenValConversion, BinaryValue) {
  {
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kBinary));
    ASSERT_TRUE(converted);
    // clang-format incorrectly adds a space after "instanceof".
    // clang-format off
    ASSERT_TRUE(converted->instanceof(emscripten::val::global("ArrayBuffer")));
    // clang-format on
    EXPECT_EQ((*converted)["byteLength"].as<int>(), 0);
  }

  {
    const std::vector<uint8_t> kBinary = {1, 2, 3};
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(kBinary));
    ASSERT_TRUE(converted);
    // clang-format incorrectly adds a space after "instanceof".
    // clang-format off
    ASSERT_TRUE(converted->instanceof(emscripten::val::global("ArrayBuffer")));
    // clang-format on
    const emscripten::val uint8_array =
        emscripten::val::global("Uint8Array").new_(*converted);
    ASSERT_EQ(uint8_array["length"].as<size_t>(), kBinary.size());
    for (size_t i = 0; i < kBinary.size(); ++i)
      EXPECT_EQ(uint8_array[i].as<int>(), kBinary[i]);
  }
}

TEST(ValueEmscriptenValConversion, DictionaryValue) {
  {
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kDictionary));
    ASSERT_TRUE(converted);
    ASSERT_EQ(converted->typeOf().as<std::string>(), "object");
    EXPECT_EQ(GetEmscriptenObjectValSize(*converted), 0);
  }

  {
    // The test data is: {"xyz": {"foo": null, "bar": 123}}.
    std::map<std::string, Value> inner_items;
    inner_items["foo"] = Value();
    inner_items["bar"] = Value(123);
    std::map<std::string, Value> items;
    items["xyz"] = Value(std::move(inner_items));
    const Value value(std::move(items));

    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(value);
    ASSERT_TRUE(converted);
    ASSERT_EQ(converted->typeOf().as<std::string>(), "object");
    EXPECT_EQ(GetEmscriptenObjectValSize(*converted), 1);
    const emscripten::val inner_dict = (*converted)["xyz"];
    ASSERT_EQ(inner_dict.typeOf().as<std::string>(), "object");
    EXPECT_EQ(GetEmscriptenObjectValSize(inner_dict), 2);
    const emscripten::val inner_item_foo = inner_dict["foo"];
    EXPECT_TRUE(inner_item_foo.isNull());
    const emscripten::val inner_item_bar = inner_dict["bar"];
    EXPECT_TRUE(inner_item_bar.isNumber());
    EXPECT_EQ(inner_item_bar.as<int>(), 123);
  }
}

TEST(ValueEmscriptenValConversion, DictionaryValueError) {
  {
    constexpr int64_t k64BitMax = std::numeric_limits<int64_t>::max();
    std::map<std::string, Value> items;
    items["foo"] = Value(k64BitMax);
    const Value value(std::move(items));

    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(value);
    EXPECT_FALSE(converted);
  }

  {
    constexpr int64_t k64BitMin = std::numeric_limits<int64_t>::min();
    std::map<std::string, Value> items;
    items["abc"] = Value();
    items["def"] = Value(k64BitMin);
    const Value value(std::move(items));

    std::string error_message;
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(value, &error_message);
    EXPECT_FALSE(converted);
    EXPECT_THAT(
        error_message,
        MatchesRegex(
            "Cannot convert dictionary item def to Emscripten val: The integer "
            "-9223372036854775808 cannot be converted into a floating-point "
            "double value without loss of precision: it is outside .* range"));
  }
}

TEST(ValueEmscriptenValConversion, ArrayValue) {
  {
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kArray));
    ASSERT_TRUE(converted);
    ASSERT_TRUE(converted->isArray());
    EXPECT_EQ((*converted)["length"].as<int>(), 0);
  }

  {
    // The test data is: [[null, 123]].
    std::vector<Value> inner_items;
    inner_items.emplace_back();
    inner_items.emplace_back(123);
    std::vector<Value> items;
    items.emplace_back(std::move(inner_items));
    const Value value(std::move(items));

    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(value);
    ASSERT_TRUE(converted);
    ASSERT_TRUE(converted->isArray());
    ASSERT_EQ((*converted)["length"].as<int>(), 1);
    const emscripten::val item0 = (*converted)[0];
    ASSERT_TRUE(item0.isArray());
    ASSERT_EQ(item0["length"].as<int>(), 2);
    const emscripten::val inner_item0 = item0[0];
    EXPECT_TRUE(inner_item0.isNull());
    const emscripten::val inner_item1 = item0[1];
    ASSERT_TRUE(inner_item1.isNumber());
    EXPECT_EQ(inner_item1.as<int>(), 123);
  }
}

TEST(ValueEmscriptenValConversion, ArrayValueError) {
  {
    constexpr int64_t k64BitMax = std::numeric_limits<int64_t>::max();
    std::vector<Value> items;
    items.emplace_back(k64BitMax);
    const Value value(std::move(items));

    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(value);
    EXPECT_FALSE(converted);
  }

  {
    constexpr int64_t k64BitMin = std::numeric_limits<int64_t>::min();
    std::vector<Value> items;
    items.emplace_back();
    items.emplace_back(k64BitMin);
    const Value value(std::move(items));

    std::string error_message;
    const std::optional<emscripten::val> converted =
        ConvertValueToEmscriptenVal(value, &error_message);
    EXPECT_FALSE(converted);
    EXPECT_THAT(
        error_message,
        MatchesRegex(
            "Cannot convert array item #1 to Emscripten val: The integer "
            "-9223372036854775808 cannot be converted into a floating-point "
            "double value without loss of precision: it is outside .* range"));
  }
}

TEST(ValueEmscriptenValConversion, UndefinedEmscriptenVal) {
  std::string error_message;
  const std::optional<Value> value =
      ConvertEmscriptenValToValue(emscripten::val::undefined(), &error_message);
  EXPECT_TRUE(error_message.empty());
  ASSERT_TRUE(value);
  EXPECT_TRUE(value->is_null());
}

TEST(ValueEmscriptenValConversion, NullEmscriptenVal) {
  std::string error_message;
  const std::optional<Value> value =
      ConvertEmscriptenValToValue(emscripten::val::null(), &error_message);
  EXPECT_TRUE(error_message.empty());
  ASSERT_TRUE(value);
  EXPECT_TRUE(value->is_null());
}

TEST(ValueEmscriptenValConversion, BooleanEmscriptenVal) {
  {
    constexpr bool kBoolean = false;
    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val(kBoolean), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_boolean());
    EXPECT_EQ(value->GetBoolean(), kBoolean);
  }

  {
    constexpr bool kBoolean = true;
    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val(kBoolean), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_boolean());
    EXPECT_EQ(value->GetBoolean(), kBoolean);
  }
}

TEST(ValueEmscriptenValConversion, IntegerNumberEmscriptenVal) {
  {
    constexpr int kNumber = 123;
    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val(kNumber), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_integer());
    EXPECT_EQ(value->GetInteger(), kNumber);
  }

  {
    // This is a big number that is guaranteed to fit into `int` and into the
    // range of precisely representable JavaScript numbers.
    constexpr int kBig =
        sizeof(int) < 4 ? std::numeric_limits<int>::max() : (1 << 30);
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val(kBig));
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_integer());
    EXPECT_EQ(value->GetInteger(), kBig);
  }

  {
    constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();
    const std::optional<Value> value = ConvertEmscriptenValToValue(
        emscripten::val(static_cast<double>(kInt64Max)));
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_float());
    EXPECT_DOUBLE_EQ(value->GetFloat(), static_cast<double>(kInt64Max));
  }
}

TEST(ValueEmscriptenValConversion, FloatNumberEmscriptenVal) {
  {
    constexpr double kFractional = 123.456;
    std::string error_message;
    const std::optional<Value> value = ConvertEmscriptenValToValue(
        emscripten::val(kFractional), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_float());
    EXPECT_DOUBLE_EQ(value->GetFloat(), kFractional);
  }

  {
    constexpr double kHuge = 1E100;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val(kHuge));
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_float());
    EXPECT_DOUBLE_EQ(value->GetFloat(), kHuge);
  }
}

TEST(ValueEmscriptenValConversion, StringEmscriptenVal) {
  {
    constexpr char kEmpty[] = "";
    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val(kEmpty), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_string());
    EXPECT_EQ(value->GetString(), kEmpty);
  }

  {
    constexpr char kFoo[] = "foo";
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val(kFoo));
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_string());
    EXPECT_EQ(value->GetString(), kFoo);
  }
}

TEST(ValueEmscriptenValConversion, ArrayBufferEmscriptenVal) {
  {
    std::string error_message;
    const std::optional<Value> value = ConvertEmscriptenValToValue(
        emscripten::val::global("ArrayBuffer").new_(), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_binary());
    EXPECT_TRUE(value->GetBinary().empty());
  }

  {
    const std::vector<uint8_t> kBytes = {1, 2, 3};
    const emscripten::val uint8_array_val =
        emscripten::val::global("Uint8Array")
            .call<emscripten::val>("from", emscripten::val::array(kBytes));
    emscripten::val array_buffer_val = uint8_array_val["buffer"];

    const std::optional<Value> value =
        ConvertEmscriptenValToValue(array_buffer_val);
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_binary());
    EXPECT_EQ(value->GetBinary(), kBytes);
  }
}

TEST(ValueEmscriptenValConversion, FunctionEmscriptenVal) {
  {
    emscripten::val val = emscripten::val::global("String");
    std::string error_message;
    EXPECT_FALSE(ConvertEmscriptenValToValue(val, &error_message));
    EXPECT_EQ(error_message, "Conversion error: unsupported type \"function\"");
  }

  {
    emscripten::val val = emscripten::val::global("parseInt");
    EXPECT_FALSE(ConvertEmscriptenValToValue(val));
  }
}

TEST(ValueEmscriptenValConversion, ArrayEmscriptenVal) {
  {
    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val::array(), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_array());
    EXPECT_TRUE(value->GetArray().empty());
  }

  {
    emscripten::val val = emscripten::val::global("JSON").call<emscripten::val>(
        "parse", std::string("[[null, 123]]"));

    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(val, &error_message);
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

TEST(ValueEmscriptenValConversion, ArrayEmscriptenValWithBadItem) {
  emscripten::val inner_array_val = emscripten::val::array();  // [<function>]
  inner_array_val.set(0, emscripten::val::global("parseInt"));
  emscripten::val array_val = emscripten::val::array();  // [null, [<function>]]
  array_val.set(0, emscripten::val::null());
  array_val.set(1, inner_array_val);

  EXPECT_FALSE(ConvertEmscriptenValToValue(inner_array_val));
  EXPECT_FALSE(ConvertEmscriptenValToValue(array_val));

  {
    std::string error_message;
    EXPECT_FALSE(ConvertEmscriptenValToValue(inner_array_val, &error_message));
    EXPECT_EQ(error_message,
              "Error converting array item #0: Conversion error: unsupported "
              "type \"function\"");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertEmscriptenValToValue(array_val, &error_message));
    EXPECT_EQ(error_message,
              "Error converting array item #1: Error converting array item #0: "
              "Conversion error: unsupported type \"function\"");
  }
}

TEST(ValueEmscriptenValConversion, Uint8ArrayEmscriptenVal) {
  {
    std::string error_message;
    const std::optional<Value> value = ConvertEmscriptenValToValue(
        emscripten::val::global("Uint8Array").new_(), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_array());
    EXPECT_TRUE(value->GetArray().empty());
  }

  {
    const std::vector<uint8_t> kBytes = {1, 2, 255};
    emscripten::val val = emscripten::val::global("Uint8Array")
                              .new_(emscripten::val::array(kBytes));

    const std::optional<Value> value = ConvertEmscriptenValToValue(val);
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_array());
    ASSERT_EQ(value->GetArray().size(), kBytes.size());
    for (size_t i = 0; i < kBytes.size(); ++i) {
      const Value& item = *value->GetArray()[i];
      ASSERT_TRUE(item.is_integer());
      EXPECT_EQ(item.GetInteger(), kBytes[i]);
    }
  }
}

TEST(ValueEmscriptenValConversion, DataViewEmscriptenVal) {
  {
    const emscripten::val array_buffer_val =
        emscripten::val::global("ArrayBuffer").new_();
    const emscripten::val data_view_val =
        emscripten::val::global("DataView").new_(array_buffer_val);

    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(data_view_val, &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_binary());
    EXPECT_TRUE(value->GetBinary().empty());
  }

  {
    const std::vector<uint8_t> kBytes = {1, 2, 3};
    const emscripten::val uint8_array_val =
        emscripten::val::global("Uint8Array")
            .call<emscripten::val>("from", emscripten::val::array(kBytes));
    emscripten::val array_buffer_val = uint8_array_val["buffer"];
    emscripten::val data_view_val =
        emscripten::val::global("DataView").new_(array_buffer_val);

    const std::optional<Value> value =
        ConvertEmscriptenValToValue(data_view_val);
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_binary());
    EXPECT_EQ(value->GetBinary(), kBytes);
  }

  {
    const std::vector<uint8_t> kBytes = {1, 2, 3};
    const emscripten::val uint8_array_val =
        emscripten::val::global("Uint8Array")
            .call<emscripten::val>("from", emscripten::val::array(kBytes));
    emscripten::val array_buffer_val = uint8_array_val["buffer"];
    emscripten::val data_view_val =
        emscripten::val::global("DataView")
            .new_(array_buffer_val, /*byteLength=*/1, /*byteOffset=*/1);

    const std::optional<Value> value =
        ConvertEmscriptenValToValue(data_view_val);
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_binary());
    EXPECT_EQ(value->GetBinary(), std::vector<uint8_t>({2}));
  }
}

TEST(ValueEmscriptenValConversion, ObjectEmscriptenVal) {
  {
    std::string error_message;
    const std::optional<Value> value =
        ConvertEmscriptenValToValue(emscripten::val::object(), &error_message);
    EXPECT_TRUE(error_message.empty());
    ASSERT_TRUE(value);
    ASSERT_TRUE(value->is_dictionary());
    EXPECT_TRUE(value->GetDictionary().empty());
  }

  {
    emscripten::val val = emscripten::val::global("JSON").call<emscripten::val>(
        "parse", std::string(R"({"xyz": {"foo": null, "bar": 123}})"));

    const std::optional<Value> value = ConvertEmscriptenValToValue(val);
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

TEST(ValueEmscriptenValConversion, ObjectEmscriptenValWithBadItem) {
  emscripten::val inner_object_val =
      emscripten::val::object();  // {"someInnerKey": <function>}
  inner_object_val.set("someInnerKey", emscripten::val::global("parseInt"));
  emscripten::val object_val =
      emscripten::val::object();  // {"someKey": {"someInnerKey": <function>}}
  object_val.set("someKey", inner_object_val);

  EXPECT_FALSE(ConvertEmscriptenValToValue(inner_object_val));
  EXPECT_FALSE(ConvertEmscriptenValToValue(object_val));

  {
    std::string error_message;
    EXPECT_FALSE(ConvertEmscriptenValToValue(inner_object_val, &error_message));
    EXPECT_EQ(error_message,
              "Error converting object property \"someInnerKey\": Conversion "
              "error: unsupported type \"function\"");
  }

  {
    std::string error_message;
    EXPECT_FALSE(ConvertEmscriptenValToValue(object_val, &error_message));
    EXPECT_EQ(error_message,
              "Error converting object property \"someKey\": Error converting "
              "object property \"someInnerKey\": Conversion error: unsupported "
              "type \"function\"");
  }
}

// Test that `ConvertEmscriptenValToValueOrDie()` succeeds on supported inputs.
// As death tests aren't supported, we don't test failure scenarios.
TEST(ValueEmscriptenValConversion, EmscriptenValOrDie) {
  {
    constexpr bool kBoolean = false;
    const Value value =
        ConvertEmscriptenValToValueOrDie(emscripten::val(kBoolean));
    ASSERT_TRUE(value.is_boolean());
    EXPECT_EQ(value.GetBoolean(), kBoolean);
  }

  {
    const int kInteger = 123;
    const Value value =
        ConvertEmscriptenValToValueOrDie(emscripten::val(kInteger));
    ASSERT_TRUE(value.is_integer());
    EXPECT_EQ(value.GetInteger(), kInteger);
  }

  {
    emscripten::val object_val = emscripten::val::object();
    object_val.set("foo", emscripten::val::null());

    const Value value = ConvertEmscriptenValToValueOrDie(object_val);
    ASSERT_TRUE(value.is_dictionary());
    EXPECT_EQ(value.GetDictionary().size(), 1U);
    const Value* foo_value = value.GetDictionaryItem("foo");
    ASSERT_TRUE(foo_value);
    EXPECT_TRUE(foo_value->is_null());
  }
}

}  // namespace google_smart_card
