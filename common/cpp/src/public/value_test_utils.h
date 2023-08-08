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

#include <sstream>
#include <string>
#include <utility>

#include <gmock/gmock.h>

#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"
#include "common/cpp/src/public/value_debug_dumping.h"

namespace google_smart_card {

inline void PrintTo(const Value& value, std::ostream* os) {
  *os << DebugDumpValueFull(value);
}

MATCHER_P(StrictlyEquals,
          value,
          /*description_string=*/std::string("strictly equals to ") +
              ::testing::PrintToString(value)) {
  return arg.StrictlyEquals(ConvertToValueOrDie(std::move(value)));
}

MATCHER_P(DictSizeIs,
          size,
          /*description_string=*/std::string("dictionary is of size ") +
              ::testing::PrintToString(size)) {
  // Cast both sizes to `int` to avoid compiler warnings when the test passes a
  // signed `size` (not casting to `size_t` in order to fail if the test goes
  // wild and passes a negative size).
  return arg.is_dictionary() &&
         static_cast<int>(arg.GetDictionary().size()) == static_cast<int>(size);
}

MATCHER_P2(DictContains,
           key,
           value,
           /*description_string=*/std::string("dictionary has key \"") + key +
               "\" with value " + ::testing::PrintToString(value)) {
  return arg.is_dictionary() && arg.GetDictionaryItem(key) &&
         arg.GetDictionaryItem(key)->StrictlyEquals(
             ConvertToValueOrDie(std::move(value)));
}

}  // namespace google_smart_card
