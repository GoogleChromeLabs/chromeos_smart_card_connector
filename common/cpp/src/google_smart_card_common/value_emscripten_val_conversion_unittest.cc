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

#include <google_smart_card_common/value_emscripten_val_conversion.h>

#include <stdint.h>

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <emscripten/val.h>
#include <gtest/gtest.h>

#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

namespace {

int GetEmscriptenObjectValSize(const emscripten::val& val) {
  const emscripten::val keys =
      emscripten::val::global("Object").call<emscripten::val>("keys", val);
  return keys["length"].as<int>();
}

}  // namespace

TEST(ValueEmscriptenValConversion, NullValue) {
  const emscripten::val converted = ConvertValueToEmscriptenVal(Value());
  EXPECT_TRUE(converted.isNull());
}

TEST(ValueEmscriptenValConversion, BooleanValue) {
  {
    constexpr bool kBoolean = false;
    const emscripten::val converted =
        ConvertValueToEmscriptenVal(Value(kBoolean));
    ASSERT_EQ(converted.typeOf().as<std::string>(), "boolean");
    EXPECT_EQ(converted.as<bool>(), kBoolean);
  }

  {
    constexpr bool kBoolean = true;
    const emscripten::val converted =
        ConvertValueToEmscriptenVal(Value(kBoolean));
    ASSERT_EQ(converted.typeOf().as<std::string>(), "boolean");
    EXPECT_EQ(converted.as<bool>(), kBoolean);
  }
}

TEST(ValueEmscriptenValConversion, IntegerValue) {
  constexpr int kNumber = 123;
  const emscripten::val converted = ConvertValueToEmscriptenVal(Value(kNumber));
  ASSERT_TRUE(converted.isNumber());
  EXPECT_EQ(converted.as<int>(), kNumber);
}

TEST(ValueEmscriptenValConversion, IntegerNon32BitValue) {
  constexpr int64_t k40Bit = 1LL << 40;
  const emscripten::val converted = ConvertValueToEmscriptenVal(Value(k40Bit));
  ASSERT_TRUE(converted.isNumber());
  // `emscripten::val` doesn't provide a direct way to transform into a
  // non-32-bit integer, so compare string representations.
  EXPECT_EQ(converted.call<std::string>("toString"), std::to_string(k40Bit));
}

TEST(ValueEmscriptenValConversion, Integer64BitMaxValue) {
  constexpr int64_t k64BitMax = std::numeric_limits<int64_t>::max();
  const emscripten::val converted =
      ConvertValueToEmscriptenVal(Value(k64BitMax));
  ASSERT_TRUE(converted.isNumber());
  EXPECT_DOUBLE_EQ(converted.as<double>(), static_cast<double>(k64BitMax));
}

TEST(ValueEmscriptenValConversion, Integer64BitMinValue) {
  constexpr int64_t k64BitMin = std::numeric_limits<int64_t>::min();
  const emscripten::val converted =
      ConvertValueToEmscriptenVal(Value(k64BitMin));
  ASSERT_TRUE(converted.isNumber());
  EXPECT_DOUBLE_EQ(converted.as<double>(), static_cast<double>(k64BitMin));
}

TEST(ValueEmscriptenValConversion, FloatValue) {
  constexpr double kFloat = 123.456;
  const emscripten::val converted = ConvertValueToEmscriptenVal(Value(kFloat));
  ASSERT_TRUE(converted.isNumber());
  EXPECT_DOUBLE_EQ(converted.as<double>(), static_cast<double>(kFloat));
}

TEST(ValueEmscriptenValConversion, StringValue) {
  {
    const emscripten::val converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kString));
    ASSERT_TRUE(converted.isString());
    EXPECT_EQ(converted.as<std::string>(), "");
  }

  {
    const char kFoo[] = "foo";
    const emscripten::val converted = ConvertValueToEmscriptenVal(Value(kFoo));
    ASSERT_TRUE(converted.isString());
    EXPECT_EQ(converted.as<std::string>(), kFoo);
  }
}

TEST(ValueEmscriptenValConversion, BinaryValue) {
  {
    const emscripten::val converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kBinary));
    ASSERT_TRUE(converted.instanceof (emscripten::val::global("ArrayBuffer")));
    EXPECT_EQ(converted["byteLength"].as<int>(), 0);
  }

  {
    const std::vector<uint8_t> kBinary = {1, 2, 3};
    const emscripten::val converted =
        ConvertValueToEmscriptenVal(Value(kBinary));
    ASSERT_TRUE(converted.instanceof (emscripten::val::global("ArrayBuffer")));
    const emscripten::val uint8_array =
        emscripten::val::global("Uint8Array").new_(converted);
    ASSERT_EQ(uint8_array["length"].as<size_t>(), kBinary.size());
    for (size_t i = 0; i < kBinary.size(); ++i)
      EXPECT_EQ(uint8_array[i].as<int>(), kBinary[i]);
  }
}

TEST(ValueEmscriptenValConversion, DictionaryValue) {
  {
    const emscripten::val converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kDictionary));
    ASSERT_EQ(converted.typeOf().as<std::string>(), "object");
    EXPECT_EQ(GetEmscriptenObjectValSize(converted), 0);
  }

  {
    // The test data is: {"xyz": {"foo": null, "bar": 123}}.
    std::map<std::string, std::unique_ptr<Value>> inner_items;
    inner_items["foo"] = MakeUnique<Value>();
    inner_items["bar"] = MakeUnique<Value>(123);
    std::map<std::string, std::unique_ptr<Value>> items;
    items["xyz"] = MakeUnique<Value>(std::move(inner_items));
    const Value value(std::move(items));

    const emscripten::val converted = ConvertValueToEmscriptenVal(value);
    ASSERT_EQ(converted.typeOf().as<std::string>(), "object");
    EXPECT_EQ(GetEmscriptenObjectValSize(converted), 1);
    const emscripten::val inner_dict = converted["xyz"];
    ASSERT_EQ(inner_dict.typeOf().as<std::string>(), "object");
    EXPECT_EQ(GetEmscriptenObjectValSize(inner_dict), 2);
    const emscripten::val inner_item_foo = inner_dict["foo"];
    EXPECT_TRUE(inner_item_foo.isNull());
    const emscripten::val inner_item_bar = inner_dict["bar"];
    EXPECT_TRUE(inner_item_bar.isNumber());
    EXPECT_EQ(inner_item_bar.as<int>(), 123);
  }
}

TEST(ValueEmscriptenValConversion, ArrayValue) {
  {
    const emscripten::val converted =
        ConvertValueToEmscriptenVal(Value(Value::Type::kArray));
    ASSERT_TRUE(converted.isArray());
    EXPECT_EQ(converted["length"].as<int>(), 0);
  }

  {
    // The test data is: [[null, 123]].
    std::vector<std::unique_ptr<Value>> inner_items;
    inner_items.push_back(MakeUnique<Value>());
    inner_items.push_back(MakeUnique<Value>(123));
    std::vector<std::unique_ptr<Value>> items;
    items.push_back(MakeUnique<Value>(std::move(inner_items)));
    const Value value(std::move(items));

    const emscripten::val converted = ConvertValueToEmscriptenVal(value);
    ASSERT_TRUE(converted.isArray());
    ASSERT_EQ(converted["length"].as<int>(), 1);
    const emscripten::val item0 = converted[0];
    ASSERT_TRUE(item0.isArray());
    ASSERT_EQ(item0["length"].as<int>(), 2);
    const emscripten::val inner_item0 = item0[0];
    EXPECT_TRUE(inner_item0.isNull());
    const emscripten::val inner_item1 = item0[1];
    ASSERT_TRUE(inner_item1.isNumber());
    EXPECT_EQ(inner_item1.as<int>(), 123);
  }
}

}  // namespace google_smart_card
