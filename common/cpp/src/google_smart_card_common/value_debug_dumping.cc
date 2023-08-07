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

#include "common/cpp/src/google_smart_card_common/value_debug_dumping.h"

#include <string>

#include "common/cpp/src/google_smart_card_common/logging/hex_dumping.h"
#include "common/cpp/src/google_smart_card_common/logging/logging.h"
#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

namespace {

__attribute__((unused)) std::string GetValueTypeTitle(const Value& value) {
  switch (value.type()) {
    case Value::Type::kNull:
      return Value::kNullTypeTitle;
    case Value::Type::kBoolean:
      return Value::kBooleanTypeTitle;
    case Value::Type::kInteger:
      return Value::kIntegerTypeTitle;
    case Value::Type::kFloat:
      return Value::kFloatTypeTitle;
    case Value::Type::kString:
      return Value::kStringTypeTitle;
    case Value::Type::kBinary:
      return Value::kBinaryTypeTitle;
    case Value::Type::kDictionary:
      return Value::kDictionaryTypeTitle;
    case Value::Type::kArray:
      return Value::kArrayTypeTitle;
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

std::string DebugDumpBoolean(bool value) {
  return value ? "true" : "false";
}

std::string DebugDumpString(const std::string& value) {
  return '"' + value + '"';
}

std::string DebugDumpArray(const Value::ArrayStorage& array_value) {
  std::string result = "[";
  for (size_t index = 0; index < array_value.size(); ++index) {
    if (index > 0)
      result += ", ";
    result += DebugDumpValueFull(*array_value[index]);
  }
  result += "]";
  return result;
}

std::string DebugDumpDictionary(
    const Value::DictionaryStorage& dictionary_value) {
  std::string result = "{";
  bool is_first_item = true;
  for (const auto& item : dictionary_value) {
    const std::string& item_key = item.first;
    const Value& item_value = *item.second;
    if (!is_first_item)
      result += ", ";
    is_first_item = false;
    result += DebugDumpString(item_key);
    result += ": ";
    result += DebugDumpValueFull(item_value);
  }
  result += "}";
  return result;
}

std::string DebugDumpBinary(const Value::BinaryStorage& binary_value) {
  // Put the type title in front of the dump, so that it can be distinguished
  // from a dump of an array value. We don't put the title in all other cases,
  // since all of them can be unambiguously interpreted based on their format,
  // and for the sake of keeping the dumps easy to read.
  std::string result = std::string(Value::kBinaryTypeTitle) + "[";
  for (size_t offset = 0; offset < binary_value.size(); ++offset) {
    if (offset > 0)
      result += ", ";
    result += HexDumpByte(binary_value[offset]);
  }
  result += "]";
  return result;
}

}  // namespace

std::string DebugDumpValueSanitized(const Value& value) {
#ifdef NDEBUG
  return GetValueTypeTitle(value);
#else
  return DebugDumpValueFull(value);
#endif
}

std::string DebugDumpValueFull(const Value& value) {
  switch (value.type()) {
    case Value::Type::kNull:
      return Value::kNullTypeTitle;
    case Value::Type::kBoolean:
      return DebugDumpBoolean(value.GetBoolean());
    case Value::Type::kInteger:
      return HexDumpUnknownSizeInteger(value.GetInteger());
    case Value::Type::kFloat:
      return std::to_string(value.GetFloat());
    case Value::Type::kString:
      return DebugDumpString(value.GetString());
    case Value::Type::kBinary:
      return DebugDumpBinary(value.GetBinary());
    case Value::Type::kDictionary:
      return DebugDumpDictionary(value.GetDictionary());
    case Value::Type::kArray:
      return DebugDumpArray(value.GetArray());
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

}  // namespace google_smart_card
