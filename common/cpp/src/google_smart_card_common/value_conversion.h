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

// Helpers for converting between `Value` instances and usual non-variant C++
// types:
// * "Value ConvertToValue(T object)";
// * "bool ConvertFromValue(Value value, T* object,
//                          std::string* error_message = nullptr)";
// * "optional<T> ConvertFromValue(Value value,
//                                 std::string* error_message = nullptr)".
//
// These helpers are implemented for many standard types:
// * `bool`;
// * `int`, `int64_t` and other similar integer types (note: there's also a
//   special case that an integer can be converted from a floating-point
//   `Value`, in case it lies within the range of precisely representable);
// * `double`;
// * `std::string`.
//
// TODO: Add support for std::vector, enum, struct.

#ifndef GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_
#define GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_

#include <stdint.h>

#include <string>
#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

///////////// ConvertToValue /////////////////

// Group of overloads that perform trivial conversions to `Value`.
//
// Note: There are intentionally no specializations for constructing from
// `Value::BinaryStorage`, `Value::ArrayStorage`, `Value::DictionaryStorage`,
// since the first two are handled via a generic `std::vector` overload below,
// and the last one isn't useful in this context (as helpers in this file are
// about converting between a `Value` and a non-`Value` object).
inline Value ConvertToValue(bool boolean) { return Value(boolean); }
inline Value ConvertToValue(int number) { return Value(number); }
Value ConvertToValue(unsigned number);
inline Value ConvertToValue(long number) {
  return Value(static_cast<int64_t>(number));
}
Value ConvertToValue(unsigned long number);
inline Value ConvertToValue(uint8_t number) { return Value(number); }
inline Value ConvertToValue(int64_t number) { return Value(number); }
inline Value ConvertToValue(double number) { return Value(number); }
Value ConvertToValue(const char* characters);
inline Value ConvertToValue(std::string characters) {
  return Value(std::move(characters));
}
// Forbid conversion of pointers other than `const char*`. Without this deleted
// overload, the `bool`-argument overload would be silently picked up.
Value ConvertToValue(void* pointer_value) = delete;

///////////// ConvertFromValue ///////////////

// Group of overloads that perform trivial conversions from `Value`.
bool ConvertFromValue(Value value, bool* boolean,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, int* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, unsigned* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, long* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, unsigned long* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, uint8_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, int64_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, double* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, std::string* characters,
                      std::string* error_message = nullptr);

// Equivalent to other `ConvertFromValue()` functions, but returns result via
// an optional.
template <typename T>
inline optional<T> ConvertFromValue(Value value,
                                    std::string* error_message = nullptr) {
  T object;
  if (!ConvertFromValue(std::move(value), &object, error_message)) return {};
  return std::move(object);
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_
