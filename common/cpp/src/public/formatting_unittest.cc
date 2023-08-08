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

#include "common/cpp/src/public/formatting.h"

#include <string>

#include <gtest/gtest.h>

namespace google_smart_card {

TEST(FormatPrintfTemplateAndSet, Basic) {
  {
    std::string formatted;
    FormatPrintfTemplateAndSet(&formatted, /*format=*/"");
    EXPECT_EQ(formatted, "");
  }

  {
    std::string formatted;
    FormatPrintfTemplateAndSet(&formatted, "%d", 123);
    EXPECT_EQ(formatted, "123");
  }

  {
    std::string formatted;
    FormatPrintfTemplateAndSet(&formatted, "string=%s int=%d", "foo", 123);
    EXPECT_EQ(formatted, "string=foo int=123");
  }
}

// Test that FormatPrintfTemplateAndSet() doesn't crash when the output string
// is a null pointer.
TEST(FormatPrintfTemplateAndSet, NullPointer) {
  FormatPrintfTemplateAndSet(/*output_string=*/nullptr, /*format=*/"");
  FormatPrintfTemplateAndSet(/*output_string=*/nullptr, "%d", 123);
  FormatPrintfTemplateAndSet(/*output_string=*/nullptr, "string=%s int=%d",
                             "foo", 123);
}

// Test that FormatPrintfTemplateAndSet() correctly handles the case when the
// resulting string is quite long.
TEST(FormatPrintfTemplateAndSet, HugeResult) {
  constexpr int kLength = 100 * 1000;
  const std::string kParameter(kLength, 'a');
  std::string formatted;
  FormatPrintfTemplateAndSet(&formatted, "%s", kParameter.c_str());
  EXPECT_EQ(formatted, kParameter);
}

}  // namespace google_smart_card
