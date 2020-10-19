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
// * "bool ConvertToValue(T object, Value* value,
//                        std::string* error_message = nullptr)";
// * "bool ConvertFromValue(Value value, T* object,
//                          std::string* error_message = nullptr)".
//
// These helpers are implemented for many standard types:
// * `bool`;
// * `int`, `int64_t` and other similar integer types (note: there's also a
//   special case that an integer can be converted from a floating-point
//   `Value`, in case it's within the range of precisely representable numbers);
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

inline bool ConvertToValue(bool boolean, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(boolean);
  return true;
}

inline bool ConvertToValue(int number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

bool ConvertToValue(unsigned number, Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(long number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(static_cast<int64_t>(number));
  return true;
}

bool ConvertToValue(unsigned long number, Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(uint8_t number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

inline bool ConvertToValue(int64_t number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

inline bool ConvertToValue(double number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

// Note: `characters` must be non-null.
bool ConvertToValue(const char* characters, Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(std::string characters, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(std::move(characters));
  return true;
}

// Forbid conversion of pointers other than `const char*`. Without this deleted
// overload, the `bool`-argument overload would be silently picked up.
Value ConvertToValue(const void* pointer_value, Value* value,
                     std::string* error_message = nullptr) = delete;

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

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_
