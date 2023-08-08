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

#include "common/cpp/src/public/logging/logging.h"

#include <gtest/gtest.h>

namespace google_smart_card {

namespace {

// A separate function for the failing check, in order to have a predictable
// message in the test assertions below.
void FailCheck() {
  GOOGLE_SMART_CARD_CHECK(1 == 2);
}

// A separate function for the failing notreached, in order to have a
// predictable message in the test assertions below.
void HitNotreached() {
  GOOGLE_SMART_CARD_NOTREACHED;
}

}  // namespace

TEST(LoggingTest, Check) {
  GOOGLE_SMART_CARD_CHECK(1 == 1);
}

TEST(LoggingDeathTest, CheckFailure) {
  EXPECT_DEATH_IF_SUPPORTED(
      { FailCheck(); },
      R"(\[FATAL\] Check "1 == 2" failed. File ".*logging_unittest.cc", line .+, function "FailCheck")");
}

TEST(LoggingDeathTest, NotreachedHit) {
  EXPECT_DEATH_IF_SUPPORTED(
      { HitNotreached(); },
      R"(\[FATAL\] NOTREACHED reached in file ".*logging_unittest.cc", line .+, function "HitNotreached")");
}

}  // namespace google_smart_card
