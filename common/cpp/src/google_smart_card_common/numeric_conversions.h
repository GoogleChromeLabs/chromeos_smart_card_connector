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

// This file contains low-level operations for conversions between numeric data
// types.
//
// FIXME(emaxx): Use boost::numeric?

#ifndef GOOGLE_SMART_CARD_COMMON_NUMERIC_CONVERSIONS_H_
#define GOOGLE_SMART_CARD_COMMON_NUMERIC_CONVERSIONS_H_

#include <inttypes.h>
#include <stdint.h>

#include <limits>
#include <string>
#include <type_traits>

#include <google_smart_card_common/formatting.h>

namespace google_smart_card {

namespace internal {

// The range of integer numbers that can be represented by the double type
// exactly (this is calculated from the bit length of the mantissa, taking into
// account that one of the bits is a sign bit)
extern const int64_t kDoubleExactRangeMax;
extern const int64_t kDoubleExactRangeMin;

}  // namespace internal

// Performs safe cast of double value into a 64-bit integer value (fails if the
// value is outside the range of integers that can be represented by the double
// type exactly).
bool CastDoubleToInt64(double value, int64_t* result,
                       std::string* error_message = nullptr);

namespace internal {

template <typename T>
inline int GetIntegerSign(T value) {
  if (!value) return 0;
  return value > 0 ? +1 : -1;
}

}  // namespace internal

// Compares two integers of possibly different types, returning -1 or 0 or 1
// when, correspondingly, the first one is smaller, equal to or greater than
// the second one.
template <typename T1, typename T2>
inline int CompareIntegers(T1 value_1, T2 value_2) {
  static_assert(std::is_integral<T1>::value,
                "CompareIntegers should be used with integral types");
  static_assert(std::is_integral<T2>::value,
                "CompareIntegers should be used with integral types");
  // We have to perform the comparisons carefully due to the C++ promotion
  // rules: if comparing a signed and an unsigned value, the signed value will
  // be converted into an unsigned, leading to unexpected behavior for negative
  // numbers.
  const int sign_1 = internal::GetIntegerSign(value_1);
  const int sign_2 = internal::GetIntegerSign(value_2);
  if (sign_1 != sign_2) return sign_1 < sign_2 ? -1 : +1;
  using CommonType = typename std::common_type<T1, T2>::type;
  const CommonType promoted_value_1 = static_cast<CommonType>(value_1);
  const CommonType promoted_value_2 = static_cast<CommonType>(value_2);
  if (promoted_value_1 == promoted_value_2) return 0;
  return promoted_value_1 < promoted_value_2 ? -1 : +1;
}

// Performs safe cast of an integer value into another integer value, possibly
// of different type (fails if the value is outside the target type range).
template <typename SourceType, typename TargetType>
inline bool CastInteger(SourceType source_value, const char* target_type_name,
                        TargetType* target_value,
                        std::string* error_message = nullptr) {
  static_assert(std::is_integral<SourceType>::value,
                "CastInteger should be used with integral types");
  static_assert(std::is_integral<TargetType>::value,
                "CastInteger should be used with integral types");
  if (CompareIntegers(std::numeric_limits<TargetType>::min(), source_value) <=
          0 &&
      CompareIntegers(std::numeric_limits<TargetType>::max(), source_value) >=
          0) {
    *target_value = static_cast<TargetType>(source_value);
    return true;
  }
  FormatPrintfTemplateAndSet(
      error_message,
      "The integer value is outside the range of type \"%s\": %s not in [%s; "
      "%s] range",
      target_type_name, std::to_string(source_value).c_str(),
      std::to_string(std::numeric_limits<TargetType>::min()).c_str(),
      std::to_string(std::numeric_limits<TargetType>::max()).c_str());

  return false;
}

// Performs safe cast of integer value into a double value (fails if the value
// is outside the range of integers that can be represented by the double type
// exactly).
template <typename T>
inline bool CastIntegerToDouble(T value, double* result,
                                std::string* error_message) {
  if (CompareIntegers(internal::kDoubleExactRangeMin, value) <= 0 &&
      CompareIntegers(internal::kDoubleExactRangeMax, value) >= 0) {
    *result = static_cast<double>(value);
    return true;
  }
  FormatPrintfTemplateAndSet(
      error_message,
      "The integer %s cannot be converted into a floating-point double value "
      "without loss of precision: it is outside [%" PRId64 "; %" PRId64
      "] range",
      std::to_string(value).c_str(), internal::kDoubleExactRangeMin,
      internal::kDoubleExactRangeMax);
  return false;
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_NUMERIC_CONVERSIONS_H_
