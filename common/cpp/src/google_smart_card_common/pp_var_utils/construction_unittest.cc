// Copyright 2016 Google Inc. All Rights Reserved.
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

#include <string>

#include <gtest/gtest.h>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/pp_var_utils/construction.h>

namespace google_smart_card {

TEST(PpVarUtilsConstructionTest, CleaningUpInvalidCharacters) {
  EXPECT_EQ(" ", CleanupStringForVar("\x80"));
  EXPECT_EQ(" ", CleanupStringForVar("\xff"));
  EXPECT_EQ("       ", CleanupStringForVar("\a\b\f\n\r\t\v"));
  EXPECT_EQ("  ", CleanupStringForVar("\xd0\xb0"));  // cyrillic small letter a

  EXPECT_EQ("azAZ019", CleanupStringForVar("azAZ019"));
  EXPECT_EQ("'\"?\\_-()[]<>", CleanupStringForVar("'\"?\\_-()[]<>"));

  EXPECT_EQ("a b c d e", CleanupStringForVar("a\1b\2c\3d\4e"));
}

TEST(PpVarUtilsConstructionTest, StringConversion) {
  EXPECT_EQ("azAZ019", MakeVar("azAZ019").AsString());
  EXPECT_EQ("'\"?\\_-()[]<>", MakeVar("'\"?\\_-()[]<>").AsString());
}

}  // namespace google_smart_card
