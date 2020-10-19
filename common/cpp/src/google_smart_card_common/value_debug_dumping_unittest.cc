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

#include <google_smart_card_common/value_debug_dumping.h>

#include <stdint.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

TEST(ValueDebugDumpingTest, Null) {
  EXPECT_EQ(DebugDumpValueSanitized(Value()), Value::kNullTypeTitle);
  EXPECT_EQ(DebugDumpValueFull(Value()), Value::kNullTypeTitle);
}

TEST(ValueDebugDumpingTest, Boolean) {
  const std::string full_false = DebugDumpValueFull(Value(false));
  const std::string full_true = DebugDumpValueFull(Value(true));
  EXPECT_EQ(full_false, "false");
  EXPECT_EQ(full_true, "true");

  const std::string sanitized_false = DebugDumpValueSanitized(Value(false));
  const std::string sanitized_true = DebugDumpValueSanitized(Value(true));
#ifdef NDEBUG
  EXPECT_EQ(sanitized_false, Value::kBooleanTypeTitle);
  EXPECT_EQ(sanitized_true, Value::kBooleanTypeTitle);
#else
  EXPECT_EQ(sanitized_false, full_false);
  EXPECT_EQ(sanitized_true, full_true);
#endif
}

TEST(ValueDebugDumpingTest, Integer) {
  constexpr int64_t kInteger = 123;
  constexpr int64_t kMax = std::numeric_limits<int64_t>::max();
  constexpr int64_t kMin = std::numeric_limits<int64_t>::min();

  const std::string full_int = DebugDumpValueFull(Value(kInteger));
  const std::string full_max = DebugDumpValueFull(Value(kMax));
  const std::string full_min = DebugDumpValueFull(Value(kMin));
  EXPECT_EQ(full_int, "0x7B");
  EXPECT_EQ(full_max, "0x7FFFFFFFFFFFFFFF");
  EXPECT_EQ(full_min, "0x8000000000000000");

  const std::string sanitized_int = DebugDumpValueSanitized(Value(kInteger));
  const std::string sanitized_max = DebugDumpValueSanitized(Value(kMax));
  const std::string sanitized_min = DebugDumpValueSanitized(Value(kMin));
#ifdef NDEBUG
  EXPECT_EQ(sanitized_int, Value::kIntegerTypeTitle);
  EXPECT_EQ(sanitized_max, Value::kIntegerTypeTitle);
  EXPECT_EQ(sanitized_min, Value::kIntegerTypeTitle);
#else
  EXPECT_EQ(sanitized_int, full_int);
  EXPECT_EQ(sanitized_max, full_max);
  EXPECT_EQ(sanitized_min, full_min);
#endif
}

TEST(ValueDebugDumpingTest, Float) {
  constexpr double kFloat = 123.456;

  const std::string full_float = DebugDumpValueFull(Value(kFloat));
  EXPECT_DOUBLE_EQ(std::stod(full_float), kFloat);

  const std::string sanitized_float = DebugDumpValueSanitized(Value(kFloat));
#ifdef NDEBUG
  EXPECT_EQ(sanitized_float, Value::kFloatTypeTitle);
#else
  EXPECT_DOUBLE_EQ(std::stod(sanitized_float), kFloat);
#endif
}

TEST(ValueDebugDumpingTest, String) {
  constexpr char kFooString[] = "foo";
  const Value kBlank("");
  const Value kFoo(kFooString);

  const std::string full_blank = DebugDumpValueFull(kBlank);
  const std::string full_foo = DebugDumpValueFull(kFoo);
  EXPECT_EQ(full_blank, "\"\"");
  EXPECT_EQ(full_foo, '"' + std::string(kFooString) + '"');

  const std::string sanitized_blank = DebugDumpValueSanitized(kBlank);
  const std::string sanitized_foo = DebugDumpValueSanitized(kFoo);
#ifdef NDEBUG
  EXPECT_EQ(sanitized_blank, Value::kStringTypeTitle);
  EXPECT_EQ(sanitized_foo, Value::kStringTypeTitle);
#else
  EXPECT_EQ(sanitized_blank, full_blank);
  EXPECT_EQ(sanitized_foo, full_foo);
#endif
}

TEST(ValueDebugDumpingTest, Binary) {
  const std::vector<uint8_t> kBlobBytes = {0, 255};
  const Value kBlank(Value::Type::kBinary);
  const Value kBlob(kBlobBytes);

  const std::string full_blank = DebugDumpValueFull(kBlank);
  const std::string full_blob = DebugDumpValueFull(kBlob);
  EXPECT_EQ(full_blank, "binary[]");
  EXPECT_EQ(full_blob, "binary[0x00, 0xFF]");

  const std::string sanitized_blank = DebugDumpValueSanitized(kBlank);
  const std::string sanitized_blob = DebugDumpValueSanitized(kBlob);
#ifdef NDEBUG
  EXPECT_EQ(sanitized_blank, Value::kBinaryTypeTitle);
  EXPECT_EQ(sanitized_blob, Value::kBinaryTypeTitle);
#else
  EXPECT_EQ(sanitized_blank, full_blank);
  EXPECT_EQ(sanitized_blob, full_blob);
#endif
}

TEST(ValueDebugDumpingTest, Dictionary) {
  const Value kBlank(Value::Type::kDictionary);

  const int64_t kInteger = 123;
  Value dict(Value::Type::kDictionary);  // {"bar": null, "foo": 123}
  dict.SetDictionaryItem("bar", Value());
  dict.SetDictionaryItem("foo", Value(kInteger));

  const std::string full_blank = DebugDumpValueFull(kBlank);
  const std::string full_dict = DebugDumpValueFull(dict);
  EXPECT_EQ(full_blank, "{}");
  EXPECT_EQ(full_dict, "{\"bar\": null, \"foo\": 0x7B}");

  const std::string sanitized_blank = DebugDumpValueSanitized(kBlank);
  const std::string sanitized_dict = DebugDumpValueSanitized(dict);
#ifdef NDEBUG
  EXPECT_EQ(sanitized_blank, Value::kDictionaryTypeTitle);
  EXPECT_EQ(sanitized_dict, Value::kDictionaryTypeTitle);
#else
  EXPECT_EQ(sanitized_blank, full_blank);
  EXPECT_EQ(sanitized_dict, full_dict);
#endif
}

TEST(ValueDebugDumpingTest, Array) {
  const Value kBlank(Value::Type::kArray);

  const int64_t kInteger = 123;
  Value::ArrayStorage array_items;
  array_items.push_back(MakeUnique<Value>());
  array_items.push_back(MakeUnique<Value>(kInteger));
  const Value kArray(std::move(array_items));  // [null, 123]

  const std::string full_blank = DebugDumpValueFull(kBlank);
  const std::string full_array = DebugDumpValueFull(kArray);
  EXPECT_EQ(full_blank, "[]");
  EXPECT_EQ(full_array, "[null, 0x7B]");

  const std::string sanitized_blank = DebugDumpValueSanitized(kBlank);
  const std::string sanitized_array = DebugDumpValueSanitized(kArray);
#ifdef NDEBUG
  EXPECT_EQ(sanitized_blank, Value::kArrayTypeTitle);
  EXPECT_EQ(sanitized_array, Value::kArrayTypeTitle);
#else
  EXPECT_EQ(sanitized_blank, full_blank);
  EXPECT_EQ(sanitized_array, full_array);
#endif
}

}  // namespace google_smart_card
