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

#include "common/cpp/src/google_smart_card_common/value.h"

#include <stdint.h>

#include <limits>
#include <map>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "common/cpp/src/google_smart_card_common/unique_ptr_utils.h"

namespace google_smart_card {

TEST(ValueTest, DefaultConstructed) {
  const Value value;
  EXPECT_EQ(value.type(), Value::Type::kNull);
  EXPECT_TRUE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_TRUE(value.StrictlyEquals(Value()));
}

TEST(ValueTest, Null) {
  const Value value(Value::Type::kNull);
  EXPECT_EQ(value.type(), Value::Type::kNull);
  EXPECT_TRUE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_TRUE(value.StrictlyEquals(Value()));
}

TEST(ValueTest, Boolean) {
  const Value value(true);
  EXPECT_EQ(value.type(), Value::Type::kBoolean);
  EXPECT_FALSE(value.is_null());
  EXPECT_TRUE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());

  // Test `StrictlyEquals()` against same/different boolean values.
  EXPECT_TRUE(value.StrictlyEquals(Value(true)));
  EXPECT_FALSE(value.StrictlyEquals(Value(false)));
}

TEST(ValueTest, BooleanDefault) {
  const Value value(Value::Type::kBoolean);
  EXPECT_EQ(value.type(), Value::Type::kBoolean);
  EXPECT_FALSE(value.is_null());
  EXPECT_TRUE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());

  // Test `StrictlyEquals()` against same/different boolean values.
  EXPECT_TRUE(value.StrictlyEquals(Value(false)));
  EXPECT_FALSE(value.StrictlyEquals(Value(true)));
}

TEST(ValueTest, Integer) {
  const Value value(123);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), 123);
  EXPECT_DOUBLE_EQ(value.GetFloat(), 123);

  // Test `StrictlyEquals()` against same/different integer values.
  EXPECT_TRUE(value.StrictlyEquals(Value(123)));
  EXPECT_FALSE(value.StrictlyEquals(Value(1234)));
}

TEST(ValueTest, Integer64BitMax) {
  const int64_t integer_value = std::numeric_limits<int64_t>::max();
  const Value value(integer_value);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), integer_value);
  EXPECT_DOUBLE_EQ(value.GetFloat(), static_cast<double>(integer_value));

  // Test `StrictlyEquals()` against same/different integer values.
  EXPECT_TRUE(value.StrictlyEquals(Value(integer_value)));
  EXPECT_FALSE(value.StrictlyEquals(Value(0)));
}

TEST(ValueTest, Integer64BitMin) {
  const int64_t integer_value = std::numeric_limits<int64_t>::min();
  const Value value(integer_value);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), integer_value);
  EXPECT_DOUBLE_EQ(value.GetFloat(), integer_value);

  // Test `StrictlyEquals()` against same/different integer values.
  EXPECT_TRUE(value.StrictlyEquals(Value(integer_value)));
  EXPECT_FALSE(value.StrictlyEquals(Value(0)));
}

TEST(ValueTest, IntegerDefault) {
  const Value value(Value::Type::kInteger);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), 0);
  EXPECT_DOUBLE_EQ(value.GetFloat(), 0);

  // Test `StrictlyEquals()` against same/different integer values.
  EXPECT_TRUE(value.StrictlyEquals(Value(0)));
  EXPECT_FALSE(value.StrictlyEquals(Value(123)));
}

TEST(ValueTest, Float) {
  const Value value(123.456);
  EXPECT_EQ(value.type(), Value::Type::kFloat);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_TRUE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_DOUBLE_EQ(value.GetFloat(), 123.456);

  // Test `StrictlyEquals()` against same/different float values.
  EXPECT_TRUE(value.StrictlyEquals(Value(123.456)));
  EXPECT_FALSE(value.StrictlyEquals(Value(123.4567)));
}

TEST(ValueTest, FloatDefault) {
  const Value value(Value::Type::kFloat);
  EXPECT_EQ(value.type(), Value::Type::kFloat);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_TRUE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_DOUBLE_EQ(value.GetFloat(), 0);

  // Test `StrictlyEquals()` against same/different float values.
  EXPECT_TRUE(value.StrictlyEquals(Value(0.0)));
  EXPECT_FALSE(value.StrictlyEquals(Value(123.456)));
}

TEST(ValueTest, String) {
  const std::string kString = "foo";
  const Value value(kString);
  EXPECT_EQ(value.type(), Value::Type::kString);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetString(), kString);

  // Test `StrictlyEquals()` against same/different string values.
  EXPECT_TRUE(value.StrictlyEquals(Value(kString)));
  EXPECT_FALSE(value.StrictlyEquals(Value("bar")));
}

TEST(ValueTest, StringFromChars) {
  constexpr char kString[] = "foo";
  const Value value(kString);
  EXPECT_EQ(value.type(), Value::Type::kString);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetString(), kString);

  // Test `StrictlyEquals()` against same/different string values.
  EXPECT_TRUE(value.StrictlyEquals(Value(kString)));
  EXPECT_FALSE(value.StrictlyEquals(Value("bar")));
}

TEST(ValueTest, StringDefault) {
  const Value value(Value::Type::kString);
  EXPECT_EQ(value.type(), Value::Type::kString);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetString(), "");

  // Test `StrictlyEquals()` against same/different string values.
  EXPECT_TRUE(value.StrictlyEquals(Value("")));
  EXPECT_FALSE(value.StrictlyEquals(Value("foo")));
}

TEST(ValueTest, Binary) {
  const std::vector<uint8_t> bytes = {1, 2, 3};
  const Value value(bytes);
  EXPECT_EQ(value.type(), Value::Type::kBinary);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_TRUE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetBinary(), bytes);

  // Test `StrictlyEquals()` against same/different binary values.
  EXPECT_TRUE(value.StrictlyEquals(Value(bytes)));
  EXPECT_FALSE(value.StrictlyEquals(Value(std::vector<uint8_t>{1, 2, 3, 4})));
}

TEST(ValueTest, BinaryDefault) {
  const std::vector<uint8_t> bytes;
  const Value value(bytes);
  EXPECT_EQ(value.type(), Value::Type::kBinary);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_TRUE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetBinary(), bytes);

  // Test `StrictlyEquals()` against same/different binary values.
  EXPECT_TRUE(value.StrictlyEquals(Value(bytes)));
  EXPECT_FALSE(value.StrictlyEquals(Value(std::vector<uint8_t>{1, 2, 3})));
}

TEST(ValueTest, Dictionary) {
  std::map<std::string, std::unique_ptr<Value>> items;
  items["foo"] = MakeUnique<Value>();
  items["bar"] = MakeUnique<Value>(123);
  const Value value(std::move(items));
  EXPECT_EQ(value.type(), Value::Type::kDictionary);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetDictionary().size(), 2U);
  const Value* const item_foo = value.GetDictionaryItem("foo");
  ASSERT_TRUE(item_foo);
  EXPECT_TRUE(item_foo->is_null());
  const Value* const item_bar = value.GetDictionaryItem("bar");
  ASSERT_TRUE(item_bar);
  ASSERT_TRUE(item_bar->is_integer());
  EXPECT_EQ(item_bar->GetInteger(), 123);
  const Value* const item_baz = value.GetDictionaryItem("baz");
  EXPECT_FALSE(item_baz);

  // Test `StrictlyEquals()` against same/different dictionary values.
  std::map<std::string, std::unique_ptr<Value>> clone;
  clone["foo"] = MakeUnique<Value>();
  clone["bar"] = MakeUnique<Value>(123);
  EXPECT_TRUE(value.StrictlyEquals(Value(std::move(clone))));
  std::map<std::string, std::unique_ptr<Value>> other;
  other["foo"] = MakeUnique<Value>();
  other["bar"] = MakeUnique<Value>(1234);
  EXPECT_FALSE(value.StrictlyEquals(Value(std::move(other))));
}

TEST(ValueTest, DictionaryDefault) {
  const Value value(Value::Type::kDictionary);
  EXPECT_EQ(value.type(), Value::Type::kDictionary);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_TRUE(value.GetDictionary().empty());
  const Value* const item_foo = value.GetDictionaryItem("foo");
  EXPECT_FALSE(item_foo);

  // Test `StrictlyEquals()` against same/different dictionary values.
  EXPECT_TRUE(value.StrictlyEquals(Value(Value::Type::kDictionary)));
  std::map<std::string, std::unique_ptr<Value>> other;
  other["foo"] = MakeUnique<Value>();
  other["bar"] = MakeUnique<Value>(1234);
  EXPECT_FALSE(value.StrictlyEquals(Value(std::move(other))));
}

TEST(ValueTest, Array) {
  std::vector<std::unique_ptr<Value>> items;
  items.push_back(MakeUnique<Value>());
  items.push_back(MakeUnique<Value>(123));
  const Value value(std::move(items));
  EXPECT_EQ(value.type(), Value::Type::kArray);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_TRUE(value.is_array());
  ASSERT_EQ(value.GetArray().size(), 2U);
  const Value* item0 = value.GetArray()[0].get();
  ASSERT_TRUE(item0);
  EXPECT_TRUE(item0->is_null());
  const Value* item1 = value.GetArray()[1].get();
  ASSERT_TRUE(item1);
  ASSERT_TRUE(item1->is_integer());
  EXPECT_EQ(item1->GetInteger(), 123);

  // Test `StrictlyEquals()` against same/different array values.
  std::vector<std::unique_ptr<Value>> clone;
  clone.push_back(MakeUnique<Value>());
  clone.push_back(MakeUnique<Value>(123));
  EXPECT_TRUE(value.StrictlyEquals(Value(std::move(clone))));
  std::vector<std::unique_ptr<Value>> other;
  other.push_back(MakeUnique<Value>());
  other.push_back(MakeUnique<Value>(1234));
  EXPECT_FALSE(value.StrictlyEquals(Value(std::move(other))));
}

TEST(ValueTest, ArrayDefault) {
  const Value value(Value::Type::kArray);
  EXPECT_EQ(value.type(), Value::Type::kArray);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_boolean());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_TRUE(value.is_array());
  EXPECT_TRUE(value.GetArray().empty());

  // Test `StrictlyEquals()` against same/different array values.
  EXPECT_TRUE(value.StrictlyEquals(Value(Value::Type::kArray)));
  std::vector<std::unique_ptr<Value>> other;
  other.push_back(MakeUnique<Value>());
  other.push_back(MakeUnique<Value>(123));
  EXPECT_FALSE(value.StrictlyEquals(Value(std::move(other))));
}

// Test the `StrictlyEquals` method returns false for values of different types.
TEST(ValueTest, DifferentTypesAreNotStrictlyEqual) {
  const Value null_value;
  const Value boolean_value(true);
  const Value integer_value(123);
  const Value float_value(123.0);
  const Value string_value("123");
  const Value binary_value(std::vector<uint8_t>{1, 2, 3});
  const Value dictionary_value(Value::Type::kDictionary);
  const Value array_value(Value::Type::kArray);

  // Not using loops for saving typing, because when a test assertion fails in a
  // loop it's unclear what exactly failed.

  EXPECT_FALSE(null_value.StrictlyEquals(boolean_value));
  EXPECT_FALSE(null_value.StrictlyEquals(integer_value));
  EXPECT_FALSE(null_value.StrictlyEquals(float_value));
  EXPECT_FALSE(null_value.StrictlyEquals(string_value));
  EXPECT_FALSE(null_value.StrictlyEquals(binary_value));
  EXPECT_FALSE(null_value.StrictlyEquals(dictionary_value));
  EXPECT_FALSE(null_value.StrictlyEquals(array_value));

  EXPECT_FALSE(boolean_value.StrictlyEquals(integer_value));
  EXPECT_FALSE(boolean_value.StrictlyEquals(float_value));
  EXPECT_FALSE(boolean_value.StrictlyEquals(string_value));
  EXPECT_FALSE(boolean_value.StrictlyEquals(binary_value));
  EXPECT_FALSE(boolean_value.StrictlyEquals(dictionary_value));
  EXPECT_FALSE(boolean_value.StrictlyEquals(array_value));

  EXPECT_FALSE(integer_value.StrictlyEquals(float_value));
  EXPECT_FALSE(integer_value.StrictlyEquals(string_value));
  EXPECT_FALSE(integer_value.StrictlyEquals(binary_value));
  EXPECT_FALSE(integer_value.StrictlyEquals(dictionary_value));
  EXPECT_FALSE(integer_value.StrictlyEquals(array_value));

  EXPECT_FALSE(float_value.StrictlyEquals(string_value));
  EXPECT_FALSE(float_value.StrictlyEquals(binary_value));
  EXPECT_FALSE(float_value.StrictlyEquals(dictionary_value));
  EXPECT_FALSE(float_value.StrictlyEquals(array_value));

  EXPECT_FALSE(string_value.StrictlyEquals(binary_value));
  EXPECT_FALSE(string_value.StrictlyEquals(dictionary_value));
  EXPECT_FALSE(string_value.StrictlyEquals(array_value));

  EXPECT_FALSE(binary_value.StrictlyEquals(dictionary_value));
  EXPECT_FALSE(binary_value.StrictlyEquals(array_value));

  EXPECT_FALSE(dictionary_value.StrictlyEquals(array_value));
}

TEST(ValueTest, MoveConstruction) {
  {
    Value value1;
    Value value2 = std::move(value1);
    EXPECT_TRUE(value2.is_null());
  }

  {
    Value value1(123);
    Value value2 = std::move(value1);
    ASSERT_TRUE(value2.is_integer());
    EXPECT_EQ(value2.GetInteger(), 123);
  }

  {
    Value value1("foo");
    Value value2 = std::move(value1);
    ASSERT_TRUE(value2.is_string());
    EXPECT_EQ(value2.GetString(), "foo");
  }
}

TEST(ValueTest, MoveAssignment) {
  {
    Value value1("foo");
    Value value2;
    value2 = std::move(value1);
    ASSERT_TRUE(value2.is_string());
    EXPECT_EQ(value2.GetString(), "foo");
  }

  {
    Value value1("foo");
    Value value2(123);
    value2 = std::move(value1);
    ASSERT_TRUE(value2.is_string());
    EXPECT_EQ(value2.GetString(), "foo");
  }

  {
    Value value1("foo");
    Value value2("bar");
    value2 = std::move(value1);
    ASSERT_TRUE(value2.is_string());
    EXPECT_EQ(value2.GetString(), "foo");
  }

  {
    std::vector<std::unique_ptr<Value>> items;
    items.push_back(MakeUnique<Value>("foo"));
    Value value1(std::move(items));
    Value value2("bar");
    value2 = std::move(value1);
    ASSERT_TRUE(value2.is_array());
    EXPECT_EQ(value2.GetArray().size(), 1U);
  }
}

TEST(ValueTest, MoveAssignmentToItself) {
  Value value("foo");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
  value = std::move(value);
#pragma clang diagnostic pop
  ASSERT_TRUE(value.is_string());
  EXPECT_EQ(value.GetString(), "foo");
}

TEST(ValueTest, DictionaryModification) {
  Value value(Value::Type::kDictionary);

  value.SetDictionaryItem("foo", 123);
  EXPECT_EQ(value.GetDictionary().size(), 1U);
  {
    const Value* const item_foo = value.GetDictionaryItem("foo");
    ASSERT_TRUE(item_foo);
    ASSERT_TRUE(item_foo->is_integer());
    EXPECT_EQ(item_foo->GetInteger(), 123);
  }

  value.SetDictionaryItem("bar", Value());
  EXPECT_EQ(value.GetDictionary().size(), 2U);
  {
    const Value* const item_foo = value.GetDictionaryItem("foo");
    ASSERT_TRUE(item_foo);
    ASSERT_TRUE(item_foo->is_integer());
    EXPECT_EQ(item_foo->GetInteger(), 123);
    const Value* const item_bar = value.GetDictionaryItem("bar");
    ASSERT_TRUE(item_bar);
    EXPECT_TRUE(item_bar->is_null());
  }

  value.SetDictionaryItem("foo", "text");
  EXPECT_EQ(value.GetDictionary().size(), 2U);
  {
    const Value* const item_foo = value.GetDictionaryItem("foo");
    ASSERT_TRUE(item_foo);
    ASSERT_TRUE(item_foo->is_string());
    EXPECT_EQ(item_foo->GetString(), "text");
    const Value* const item_bar = value.GetDictionaryItem("bar");
    ASSERT_TRUE(item_bar);
    EXPECT_TRUE(item_bar->is_null());
  }
}

}  // namespace google_smart_card
