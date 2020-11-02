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

#include <inttypes.h>
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
    FormatPrintfTemplateAndSet(
        error_message, internal::kErrorWrongTypeValueConversion,
        Value::kIntegerTypeTitle, DebugDumpValueSanitized(value).c_str());
    return false;
  }
  return CastInteger(int64_number, type_name, number, error_message);
}

}  // namespace

namespace internal {

const char kErrorWrongTypeValueConversion[] =
    "Expected value of type %s, instead got: %s";
const char kErrorFromArrayValueConversion[] =
    "Cannot convert item #%d from value: %s";
const char kErrorToArrayValueConversion[] =
    "Cannot convert item #%d to value: %s";

EnumToValueConverter::EnumToValueConverter(int64_t enum_to_convert)
    : enum_to_convert_(enum_to_convert) {}

EnumToValueConverter::~EnumToValueConverter() = default;

void EnumToValueConverter::HandleItem(int64_t enum_item,
                                      const char* enum_item_name) {
  if (converted_value_ || enum_to_convert_ != enum_item) return;
  converted_value_ = Value(enum_item_name);
}

bool EnumToValueConverter::TakeConvertedValue(const char* type_name,
                                              Value* converted_value,
                                              std::string* error_message) {
  if (converted_value_) {
    *converted_value = std::move(*converted_value_);
    return true;
  }
  FormatPrintfTemplateAndSet(
      error_message,
      "Cannot convert enum %s to value: unknown integer value %" PRId64,
      type_name, enum_to_convert_);
  return false;
}

EnumFromValueConverter::EnumFromValueConverter(Value value_to_convert)
    : value_to_convert_(std::move(value_to_convert)) {}

EnumFromValueConverter::~EnumFromValueConverter() = default;

void EnumFromValueConverter::HandleItem(int64_t enum_item,
                                        const char* enum_item_name) {
  if (converted_enum_ || !value_to_convert_.is_string() ||
      value_to_convert_.GetString() != enum_item_name)
    return;
  converted_enum_ = enum_item;
}

bool EnumFromValueConverter::GetConvertedEnum(
    const char* type_name, int64_t* converted_enum,
    std::string* error_message) const {
  if (converted_enum_) {
    *converted_enum = *converted_enum_;
    return true;
  }
  FormatPrintfTemplateAndSet(
      error_message, "Cannot convert value %s to enum %s: %s",
      DebugDumpValueSanitized(value_to_convert_).c_str(), type_name,
      value_to_convert_.is_string() ? "unknown enum value"
                                    : "value is not a string");
  return false;
}

StructToValueConverterBase::StructToValueConverterBase() = default;

StructToValueConverterBase::~StructToValueConverterBase() = default;

void StructToValueConverterBase::HandleFieldConversionError(
    const char* dictionary_key_name) {
  succeeded_ = false;
  inner_error_message_ =
      FormatPrintfTemplate("Error creating property %s: %s",
                           dictionary_key_name, inner_error_message_.c_str());
}

bool StructToValueConverterBase::FinishConversion(
    const char* type_name, std::string* error_message) const {
  if (succeeded_) return true;
  FormatPrintfTemplateAndSet(error_message,
                             "Cannot convert struct %s to value: %s", type_name,
                             inner_error_message_.c_str());
  return false;
}

StructFromValueConverterBase::StructFromValueConverterBase(
    Value value_to_convert)
    : value_to_convert_(std::move(value_to_convert)) {
  if (!value_to_convert_.is_dictionary()) {
    succeeded_ = false;
    inner_error_message_ = "Value is not a dictionary";
  }
}

StructFromValueConverterBase::~StructFromValueConverterBase() = default;

bool StructFromValueConverterBase::ExtractKey(const char* dictionary_key_name,
                                              bool is_required,
                                              Value* item_value) {
  if (!succeeded_) return false;
  Value::DictionaryStorage& dict = value_to_convert_.GetDictionary();
  auto iter = dict.find(dictionary_key_name);
  if (iter != dict.end()) {
    *item_value = std::move(*iter->second);
    dict.erase(iter);
    return true;
  }
  if (!is_required) {
    // The optional field is missing from the dictionary - just skip it.
    return false;
  }
  succeeded_ = false;
  inner_error_message_ =
      FormatPrintfTemplate("Missing key \"%s\"", dictionary_key_name);
  return false;
}

void StructFromValueConverterBase::HandleFieldConversionError(
    const char* dictionary_key_name) {
  succeeded_ = false;
  inner_error_message_ =
      FormatPrintfTemplate("Error in property \"%s\": %s", dictionary_key_name,
                           inner_error_message_.c_str());
}

bool StructFromValueConverterBase::FinishConversion(
    const char* type_name, std::string* error_message) {
  if (succeeded_ && !value_to_convert_.GetDictionary().empty()) {
    succeeded_ = false;
    const std::string& first_unexpected_key =
        value_to_convert_.GetDictionary().begin()->first;
    inner_error_message_ = FormatPrintfTemplate("Unexpected key \"%s\"",
                                                first_unexpected_key.c_str());
  }
  if (succeeded_) return true;
  FormatPrintfTemplateAndSet(error_message,
                             "Cannot convert value to struct %s: %s", type_name,
                             inner_error_message_.c_str());
  return false;
}

}  // namespace internal

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

bool ConvertToValue(std::vector<uint8_t> bytes, Value* value,
                    std::string* /*error_message*/) {
  *value = Value(std::move(bytes));
  return true;
}

bool ConvertFromValue(Value value, bool* boolean, std::string* error_message) {
  if (value.is_boolean()) {
    *boolean = value.GetBoolean();
    return true;
  }
  FormatPrintfTemplateAndSet(
      error_message, internal::kErrorWrongTypeValueConversion,
      Value::kBooleanTypeTitle, DebugDumpValueSanitized(value).c_str());
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
  FormatPrintfTemplateAndSet(
      error_message, internal::kErrorWrongTypeValueConversion,
      FormatPrintfTemplate("%s or %s", Value::kIntegerTypeTitle,
                           Value::kFloatTypeTitle)
          .c_str(),
      DebugDumpValueSanitized(value).c_str());
  return false;
}

bool ConvertFromValue(Value value, std::string* characters,
                      std::string* error_message) {
  if (value.is_string()) {
    *characters = value.GetString();
    return true;
  }
  FormatPrintfTemplateAndSet(
      error_message, internal::kErrorWrongTypeValueConversion,
      Value::kStringTypeTitle, DebugDumpValueSanitized(value).c_str());
  return false;
}

bool ConvertFromValue(Value value, std::vector<uint8_t>* bytes,
                      std::string* error_message) {
  if (value.is_binary()) {
    // This is a special case that is the reason why the standard
    // array-to-vector template is overloaded by this function.
    *bytes = std::move(value.GetBinary());
    return true;
  }
  if (!value.is_array()) {
    // Note: We're creating the error message here rather than letting the
    // template call below do that, because we want to mention that the binary
    // value type would be allowed as well.
    FormatPrintfTemplateAndSet(
        error_message, internal::kErrorWrongTypeValueConversion,
        FormatPrintfTemplate("%s or %s", Value::kArrayTypeTitle,
                             Value::kBinaryTypeTitle)
            .c_str(),
        DebugDumpValueSanitized(value).c_str());
    return false;
  }
  // Delegate to the standard array-to-vector template implementation.
  return ConvertFromValue<uint8_t>(std::move(value), bytes, error_message);
}

}  // namespace google_smart_card
