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

#include <google_smart_card_common/value_conversion.h>

#include <stdint.h>

#include <limits>
#include <string>
#include <utility>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/numeric_conversions.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_debug_dumping.h>

namespace google_smart_card {

namespace {

constexpr char kErrorWrongType[] = "Expected value of type %s, instead got: %s";

template <typename T>
bool ConvertIntegerFromValue(Value value, const char* type_name, T* number,
                             std::string* error_message) {
  int64_t int64_number;
  if (value.is_integer()) {
    int64_number = value.GetInteger();
  } else if (value.is_float()) {
    // Non-32-bit numbers might arrive as floating-point `Value` objects, so
    // attempt a cast in case the value lies within the range of precisely
    // representable integers.
    if (!CastDoubleToInt64(value.GetFloat(), &int64_number, error_message))
      return false;
  } else {
    if (error_message) {
      *error_message =
          FormatPrintfTemplate(kErrorWrongType, Value::kIntegerTypeTitle,
                               DebugDumpValueSanitized(value).c_str());
    }
    return false;
  }
  return CastInteger(int64_number, type_name, number, error_message);
}

}  // namespace

bool ConvertToValue(unsigned number, Value* value, std::string* error_message) {
  int64_t int64_number;
  if (!CastInteger(number, /*target_type_name=*/"int64_t", &int64_number,
                   error_message)) {
    // The number is too big - this is possible in case `unsigned` is an 8-byte
    // type (which is theoretically possible according to the Standard, although
    // practically non-existing in real-world implementations).
    return false;
  }
  *value = Value(int64_number);
  return true;
}

bool ConvertToValue(unsigned long number, Value* value,
                    std::string* error_message) {
  int64_t int64_number;
  if (!CastInteger(number, /*target_type_name=*/"int64_t", &int64_number,
                   error_message)) {
    // The number is too big - this is possible in case `unsigned long` is an
    // 8-byte type.
    return false;
  }
  *value = Value(int64_number);
  return true;
}

bool ConvertToValue(const char* characters, Value* value,
                    std::string* /*error_message*/) {
  GOOGLE_SMART_CARD_CHECK(characters);
  *value = Value(characters);
  return true;
}

bool ConvertFromValue(Value value, bool* boolean, std::string* error_message) {
  if (value.is_boolean()) {
    *boolean = value.GetBoolean();
    return true;
  }
  if (error_message) {
    *error_message =
        FormatPrintfTemplate(kErrorWrongType, Value::kBooleanTypeTitle,
                             DebugDumpValueSanitized(value).c_str());
  }
  return false;
}

bool ConvertFromValue(Value value, int* number, std::string* error_message) {
  return ConvertIntegerFromValue(std::move(value), "int", number,
                                 error_message);
}

bool ConvertFromValue(Value value, unsigned* number,
                      std::string* error_message) {
  return ConvertIntegerFromValue(std::move(value), "unsigned", number,
                                 error_message);
}

bool ConvertFromValue(Value value, long* number, std::string* error_message) {
  return ConvertIntegerFromValue(std::move(value), "long", number,
                                 error_message);
}

bool ConvertFromValue(Value value, unsigned long* number,
                      std::string* error_message) {
  return ConvertIntegerFromValue(std::move(value), "unsigned long", number,
                                 error_message);
}

bool ConvertFromValue(Value value, uint8_t* number,
                      std::string* error_message) {
  return ConvertIntegerFromValue(std::move(value), "uint8_t", number,
                                 error_message);
}

bool ConvertFromValue(Value value, int64_t* number,
                      std::string* error_message) {
  return ConvertIntegerFromValue(std::move(value), "int64_t", number,
                                 error_message);
}

bool ConvertFromValue(Value value, double* number, std::string* error_message) {
  if (value.is_integer())
    return CastIntegerToDouble(value.GetInteger(), number, error_message);
  if (value.is_float()) {
    *number = value.GetFloat();
    return true;
  }
  if (error_message) {
    *error_message = FormatPrintfTemplate(
        kErrorWrongType,
        FormatPrintfTemplate("%s or %s", Value::kIntegerTypeTitle,
                             Value::kFloatTypeTitle)
            .c_str(),
        DebugDumpValueSanitized(value).c_str());
  }
  return false;
}

bool ConvertFromValue(Value value, std::string* characters,
                      std::string* error_message) {
  if (value.is_string()) {
    *characters = value.GetString();
    return true;
  }
  if (error_message) {
    *error_message =
        FormatPrintfTemplate(kErrorWrongType, Value::kStringTypeTitle,
                             DebugDumpValueSanitized(value).c_str());
  }
  return false;
}

}  // namespace google_smart_card
