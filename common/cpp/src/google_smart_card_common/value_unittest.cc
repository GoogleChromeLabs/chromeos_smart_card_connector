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

#include <google_smart_card_common/value.h>

#include <stdint.h>

#include <limits>
#include <map>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <google_smart_card_common/unique_ptr_utils.h>

namespace google_smart_card {

TEST(ValueTest, DefaultConstructed) {
  const Value value;
  EXPECT_EQ(value.type(), Value::Type::kNull);
  EXPECT_TRUE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
}

TEST(ValueTest, Null) {
  const Value value(Value::Type::kNull);
  EXPECT_EQ(value.type(), Value::Type::kNull);
  EXPECT_TRUE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
}

TEST(ValueTest, Integer) {
  const Value value(123);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), 123);
  EXPECT_DOUBLE_EQ(value.GetFloat(), 123);
}

TEST(ValueTest, Integer64BitMax) {
  const int64_t integer_value = std::numeric_limits<int64_t>::max();
  const Value value(integer_value);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), integer_value);
  EXPECT_DOUBLE_EQ(value.GetFloat(), integer_value);
}

TEST(ValueTest, Integer64BitMin) {
  const int64_t integer_value = std::numeric_limits<int64_t>::min();
  const Value value(integer_value);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), integer_value);
  EXPECT_DOUBLE_EQ(value.GetFloat(), integer_value);
}

TEST(ValueTest, IntegerDefault) {
  const Value value(Value::Type::kInteger);
  EXPECT_EQ(value.type(), Value::Type::kInteger);
  EXPECT_FALSE(value.is_null());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetInteger(), 0);
  EXPECT_DOUBLE_EQ(value.GetFloat(), 0);
}

TEST(ValueTest, Float) {
  const Value value(123.456);
  EXPECT_EQ(value.type(), Value::Type::kFloat);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_TRUE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_DOUBLE_EQ(value.GetFloat(), 123.456);
}

TEST(ValueTest, FloatDefault) {
  const Value value(Value::Type::kFloat);
  EXPECT_EQ(value.type(), Value::Type::kFloat);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_TRUE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_DOUBLE_EQ(value.GetFloat(), 0);
}

TEST(ValueTest, String) {
  const std::string kString = "foo";
  const Value value(kString);
  EXPECT_EQ(value.type(), Value::Type::kString);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetString(), kString);
}

TEST(ValueTest, StringFromChars) {
  constexpr char kString[] = "foo";
  const Value value(kString);
  EXPECT_EQ(value.type(), Value::Type::kString);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetString(), kString);
}

TEST(ValueTest, StringDefault) {
  const Value value(Value::Type::kString);
  EXPECT_EQ(value.type(), Value::Type::kString);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetString(), "");
}

TEST(ValueTest, Binary) {
  const std::vector<uint8_t> bytes = {1, 2, 3};
  const Value value(bytes);
  EXPECT_EQ(value.type(), Value::Type::kBinary);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_TRUE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetBinary(), bytes);
}

TEST(ValueTest, BinaryDefault) {
  const std::vector<uint8_t> bytes;
  const Value value(bytes);
  EXPECT_EQ(value.type(), Value::Type::kBinary);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_TRUE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_EQ(value.GetBinary(), bytes);
}

TEST(ValueTest, Dictionary) {
  std::map<std::string, std::unique_ptr<Value>> items;
  items["foo"] = MakeUnique<Value>();
  items["bar"] = MakeUnique<Value>(123);
  const Value value(std::move(items));
  EXPECT_EQ(value.type(), Value::Type::kDictionary);
  EXPECT_FALSE(value.is_null());
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
}

TEST(ValueTest, DictionaryDefault) {
  const Value value(Value::Type::kDictionary);
  EXPECT_EQ(value.type(), Value::Type::kDictionary);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_FALSE(value.is_array());
  EXPECT_TRUE(value.GetDictionary().empty());
  const Value* const item_foo = value.GetDictionaryItem("foo");
  EXPECT_FALSE(item_foo);
}

TEST(ValueTest, Array) {
  std::vector<std::unique_ptr<Value>> items;
  items.push_back(MakeUnique<Value>());
  items.push_back(MakeUnique<Value>(123));
  const Value value(std::move(items));
  EXPECT_EQ(value.type(), Value::Type::kArray);
  EXPECT_FALSE(value.is_null());
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
}

TEST(ValueTest, ArrayDefault) {
  const Value value(Value::Type::kArray);
  EXPECT_EQ(value.type(), Value::Type::kArray);
  EXPECT_FALSE(value.is_null());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_float());
  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_binary());
  EXPECT_FALSE(value.is_dictionary());
  EXPECT_TRUE(value.is_array());
  EXPECT_TRUE(value.GetArray().empty());
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
  value = std::move(value);
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
