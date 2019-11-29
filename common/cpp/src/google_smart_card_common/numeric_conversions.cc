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

#include <google_smart_card_common/numeric_conversions.h>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

namespace internal {

// Definitions of the constants declared in the header file
const int64_t kDoubleExactRangeMax = 1LL << std::numeric_limits<double>::digits;
const int64_t kDoubleExactRangeMin = -(1LL <<
    std::numeric_limits<double>::digits);

}  // namespace internal

bool CastDoubleToInt64(
    double value, int64_t* result, std::string* error_message) {
  if (!(internal::kDoubleExactRangeMin <= value &&
        value <= internal::kDoubleExactRangeMax)) {
    *error_message = FormatPrintfTemplate(
        "The real value is outside the exact integer representation range: %f "
        "not in [%" PRId64 "; %" PRId64 "]",
        value,
        internal::kDoubleExactRangeMin,
        internal::kDoubleExactRangeMax);
    return false;
  }
  *result = static_cast<int64_t>(value);
  GOOGLE_SMART_CARD_CHECK(internal::kDoubleExactRangeMin <= *result &&
                          *result <= internal::kDoubleExactRangeMax);
  return true;
}

}  // namespace google_smart_card
