// Copyright 2022 Google Inc. All Rights Reserved.
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

#include "common/cpp/src/public/value_builder.h"

#include <stdint.h>

#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <google_smart_card_common/value.h>

namespace google_smart_card {

TEST(ValueBuilderTest, ArrayEmpty) {
  ArrayValueBuilder builder;

  ASSERT_TRUE(builder.succeeded());
  Value result = std::move(builder).Get();
  ASSERT_TRUE(result.is_array());
  EXPECT_TRUE(result.GetArray().empty());
}

TEST(ValueBuilderTest, ArrayIntegerItem) {
  ArrayValueBuilder builder;
  std::move(builder).Add(123);

  ASSERT_TRUE(builder.succeeded());
  Value result = std::move(builder).Get();
  ASSERT_TRUE(result.is_array());
  ASSERT_EQ(result.GetArray().size(), 1U);
  ASSERT_TRUE(result.GetArray()[0]->is_integer());
  EXPECT_EQ(result.GetArray()[0]->GetInteger(), 123);
}

TEST(ValueBuilderTest, ArrayStringItem) {
  ArrayValueBuilder builder;
  std::move(builder).Add("foo");

  ASSERT_TRUE(builder.succeeded());
  Value result = std::move(builder).Get();
  ASSERT_TRUE(result.is_array());
  ASSERT_EQ(result.GetArray().size(), 1U);
  ASSERT_TRUE(result.GetArray()[0]->is_string());
  EXPECT_EQ(result.GetArray()[0]->GetString(), "foo");
}

TEST(ValueBuilderTest, ArrayMultipleItems) {
  ArrayValueBuilder builder;
  std::move(builder).Add(false).Add(Value());

  ASSERT_TRUE(builder.succeeded());
  Value result = std::move(builder).Get();
  ASSERT_TRUE(result.is_array());
  ASSERT_EQ(result.GetArray().size(), 2U);
  ASSERT_TRUE(result.GetArray()[0]->is_boolean());
  EXPECT_EQ(result.GetArray()[0]->GetBoolean(), false);
  EXPECT_TRUE(result.GetArray()[1]->is_null());
}

TEST(ValueBuilderTest, DictEmpty) {
  DictValueBuilder builder;

  ASSERT_TRUE(builder.succeeded());
  Value result = std::move(builder).Get();
  ASSERT_TRUE(result.is_dictionary());
  EXPECT_TRUE(result.GetDictionary().empty());
}

TEST(ValueBuilderTest, DictFloatItem) {
  DictValueBuilder builder;
  std::move(builder).Add("foo", 123.456);

  ASSERT_TRUE(builder.succeeded());
  Value result = std::move(builder).Get();
  ASSERT_TRUE(result.is_dictionary());
  EXPECT_EQ(result.GetDictionary().size(), 1U);
  ASSERT_TRUE(result.GetDictionaryItem("foo"));
  ASSERT_TRUE(result.GetDictionaryItem("foo")->is_float());
  EXPECT_EQ(result.GetDictionaryItem("foo")->GetFloat(), 123.456);
}

TEST(ValueBuilderTest, DictBinaryItem) {
  const std::vector<uint8_t> kBytes = {1, 2, 3};
  DictValueBuilder builder;
  std::move(builder).Add("foo", kBytes);

  ASSERT_TRUE(builder.succeeded());
  Value result = std::move(builder).Get();
  ASSERT_TRUE(result.is_dictionary());
  EXPECT_EQ(result.GetDictionary().size(), 1U);
  ASSERT_TRUE(result.GetDictionaryItem("foo"));
  ASSERT_TRUE(result.GetDictionaryItem("foo")->is_binary());
  EXPECT_EQ(result.GetDictionaryItem("foo")->GetBinary(), kBytes);
}

TEST(ValueBuilderTest, DictFailureDuplicate) {
  DictValueBuilder builder;
  std::move(builder).Add("foo", 1).Add("foo", 2);

  EXPECT_FALSE(builder.succeeded());
  EXPECT_EQ(builder.error_message(), R"(Duplicate key "foo")");
}

}  // namespace google_smart_card
