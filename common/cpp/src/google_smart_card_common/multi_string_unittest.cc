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

#include <google_smart_card_common/multi_string.h>

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::ElementsAre;
using testing::IsEmpty;

namespace google_smart_card {

TEST(MultiStringTest, CreateMultiString) {
  EXPECT_EQ(CreateMultiString({}), std::string("\0", 1));
  EXPECT_EQ(CreateMultiString({"foo"}), std::string("foo\0\0", 5));
  EXPECT_EQ(CreateMultiString({"foo", "bar"}), std::string("foo\0bar\0\0", 9));
}

TEST(MultiStringTest, ExtractMultiStringElements) {
  const std::string kEmptyMultiString("\0", 1);
  const std::string kOneItemMultiString("foo\0\0", 5);
  const std::string kTwoItemsMultiString("foo\0bar\0\0", 9);

  EXPECT_THAT(ExtractMultiStringElements(kEmptyMultiString), IsEmpty());
  EXPECT_THAT(ExtractMultiStringElements(kOneItemMultiString),
              ElementsAre("foo"));
  EXPECT_THAT(ExtractMultiStringElements(kTwoItemsMultiString),
              ElementsAre("foo", "bar"));
}

TEST(MultiStringTest, ExtractMultiStringElementsRawPointer) {
  // In the constants below we omit the multistring's ending null character,
  // because it's automatically added by the language at the end of string
  // literals. Being so precise in these constants allows to catch buffer
  // overrun bugs when these tests are run under ASan.
  constexpr const char* kEmptyMultiString = "";
  constexpr const char* kOneItemMultiString = "foo\0";
  constexpr const char* kTwoItemsMultiString = "foo\0bar\0";

  EXPECT_THAT(ExtractMultiStringElements(kEmptyMultiString), IsEmpty());
  EXPECT_THAT(ExtractMultiStringElements(kOneItemMultiString),
              ElementsAre("foo"));
  EXPECT_THAT(ExtractMultiStringElements(kTwoItemsMultiString),
              ElementsAre("foo", "bar"));
}

}  // namespace google_smart_card
