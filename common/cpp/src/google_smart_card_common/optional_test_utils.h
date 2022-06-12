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

#ifndef GOOGLE_SMART_CARD_COMMON_OPTIONAL_TEST_UTILS_H_
#define GOOGLE_SMART_CARD_COMMON_OPTIONAL_TEST_UTILS_H_

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <google_smart_card_common/optional.h>

namespace google_smart_card {

template <typename T>
inline void PrintTo(const optional<T>& value, std::ostream* os) {
  if (!value) {
    *os << "<null optional>";
    return;
  }
  *os << ::testing::PrintToString(*value);
}

// This is similar to `testing::IsFalse`, but is more self-explanatory at
// callsites.
MATCHER(IsNullOptional, /*description_string=*/"is null optional") {
  return !arg;
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_OPTIONAL_TEST_UTILS_H_
