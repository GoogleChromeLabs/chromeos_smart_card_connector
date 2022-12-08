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

#include <google_smart_card_common/join_string.h>

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace google_smart_card {
namespace {

TEST(JoinString, EmptyVector) {
  std::vector<std::string> vec;
  EXPECT_EQ(JoinStrings(vec, /*separator=*/","), "");
}

TEST(JoinString, OneItemVector) {
  std::vector<std::string> vec = {"foo"};
  EXPECT_EQ(JoinStrings(vec, /*separator=*/","), "foo");
}

TEST(JoinString, TwoItemsVector) {
  std::vector<std::string> vec = {"foo", "bar"};
  EXPECT_EQ(JoinStrings(vec, /*separator=*/","), "foo,bar");
}

TEST(JoinString, TwoItemsSet) {
  std::set<std::string> vec = {"a", "b"};
  EXPECT_EQ(JoinStrings(vec, /*separator=*/" - "), "a - b");
}

TEST(JoinString, EmptySeparator) {
  std::vector<std::string> vec = {"foo", "bar", "foo"};
  EXPECT_EQ(JoinStrings(vec, /*separator=*/""), "foobarfoo");
}

TEST(JoinString, EmptyItem) {
  std::vector<std::string> vec = {"foo", "", "bar"};
  EXPECT_EQ(JoinStrings(vec, /*separator=*/","), "foo,,bar");
}

TEST(JoinString, EmptyItemsOnly) {
  std::vector<std::string> vec = {"", "", ""};
  EXPECT_EQ(JoinStrings(vec, /*separator=*/","), ",,");
}

}  // namespace
}  // namespace google_smart_card
