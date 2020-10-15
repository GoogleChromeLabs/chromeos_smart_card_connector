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

#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

#include <stdint.h>

#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

namespace internal {

const char kUnsupportedPpVarTypeConversionError[] =
    "Error converting: unsupported type \"%s\"";
const char kPpVarDictionaryItemConversionError[] =
    "Error converting dictionary item \"%s\": %s";
const char kPpVarArrayItemConversionError[] =
    "Error converting array item #%d: %s";

}  // namespace internal

namespace {

pp::Var CreateIntegerVar(int64_t integer_value) {
  // `pp::Var` can only hold 32-bit integers. Larger numbers need to be
  // represented via double.
  if (integer_value >= std::numeric_limits<int32_t>::min() &&
      integer_value <= std::numeric_limits<int32_t>::max())
    return pp::Var(static_cast<int32_t>(integer_value));
  return pp::Var(static_cast<double>(integer_value));
}

pp::VarArrayBuffer CreateVarArrayBuffer(
    const Value::BinaryStorage& binary_storage) {
  pp::VarArrayBuffer var_array_buffer(binary_storage.size());
  if (!binary_storage.empty()) {
    std::memcpy(var_array_buffer.Map(), &binary_storage.front(),
                binary_storage.size());
    var_array_buffer.Unmap();
  }
  return var_array_buffer;
}

pp::VarDictionary CreateVarDictionary(
    const Value::DictionaryStorage& dictionary_storage) {
  pp::VarDictionary var_dictionary;
  for (const auto& item : dictionary_storage) {
    const std::string& item_key = item.first;
    const std::unique_ptr<Value>& item_value = item.second;
    GOOGLE_SMART_CARD_CHECK(
        var_dictionary.Set(item_key, ConvertValueToPpVar(*item_value)));
  }
  return var_dictionary;
}

pp::VarArray CreateVarArray(const Value::ArrayStorage& array_storage) {
  pp::VarArray var_array;
  GOOGLE_SMART_CARD_CHECK(var_array.SetLength(array_storage.size()));
  for (size_t index = 0; index < array_storage.size(); ++index) {
    const std::unique_ptr<Value>& item = array_storage[index];
    GOOGLE_SMART_CARD_CHECK(var_array.Set(index, ConvertValueToPpVar(*item)));
  }
  return var_array;
}

optional<Value> CreateValueFromPpVarArray(const pp::VarArray& var,
                                          std::string* error_message) {
  Value::ArrayStorage array_storage;
  for (uint32_t index = 0; index < var.GetLength(); ++index) {
    optional<Value> converted_item =
        ConvertPpVarToValue(var.Get(index), error_message);
    if (!converted_item) {
      if (error_message) {
        *error_message = FormatPrintfTemplate(
            internal::kPpVarArrayItemConversionError, static_cast<int>(index),
            error_message->c_str());
      }
      return {};
    }
    array_storage.push_back(MakeUnique<Value>(std::move(*converted_item)));
  }
  return Value(std::move(array_storage));
}

optional<Value> CreateValueFromPpVarDictionary(const pp::VarDictionary& var,
                                               std::string* error_message) {
  Value value(Value::Type::kDictionary);
  const pp::VarArray keys = var.GetKeys();
  for (uint32_t index = 0; index < keys.GetLength(); ++index) {
    const pp::Var item_key = keys.Get(index);
    const pp::Var item_value = var.Get(item_key);
    GOOGLE_SMART_CARD_CHECK(item_key.is_string());
    optional<Value> converted_item_value =
        ConvertPpVarToValue(item_value, error_message);
    if (!converted_item_value) {
      if (error_message) {
        *error_message = FormatPrintfTemplate(
            internal::kPpVarDictionaryItemConversionError,
            item_key.AsString().c_str(), error_message->c_str());
      }
      return {};
    }
    value.SetDictionaryItem(item_key.AsString(),
                            std::move(*converted_item_value));
  }
  return std::move(value);
}

Value CreateValueFromVarArrayBuffer(pp::VarArrayBuffer var) {
  const uint8_t* const data = static_cast<const uint8_t*>(var.Map());
  Value value(Value::BinaryStorage(data, data + var.ByteLength()));
  var.Unmap();
  return value;
}

}  // namespace

pp::Var ConvertValueToPpVar(const Value& value) {
  switch (value.type()) {
    case Value::Type::kNull:
      return pp::Var(pp::Var::Null());
    case Value::Type::kBoolean:
      return pp::Var(value.GetBoolean());
    case Value::Type::kInteger:
      return CreateIntegerVar(value.GetInteger());
    case Value::Type::kFloat:
      return pp::Var(value.GetFloat());
    case Value::Type::kString:
      return pp::Var(value.GetString());
    case Value::Type::kBinary:
      return CreateVarArrayBuffer(value.GetBinary());
    case Value::Type::kDictionary:
      return CreateVarDictionary(value.GetDictionary());
    case Value::Type::kArray:
      return CreateVarArray(value.GetArray());
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

optional<Value> ConvertPpVarToValue(const pp::Var& var,
                                    std::string* error_message) {
  if (var.is_undefined() || var.is_null()) return Value();
  if (var.is_bool()) return Value(var.AsBool());
  if (var.is_string()) return Value(var.AsString());
  if (var.is_object() || var.is_resource()) {
    if (error_message) {
      *error_message =
          FormatPrintfTemplate(internal::kUnsupportedPpVarTypeConversionError,
                               var.is_object() ? "object" : "resource");
    }
    return {};
  }
  if (var.is_array())
    return CreateValueFromPpVarArray(pp::VarArray(var), error_message);
  if (var.is_dictionary()) {
    return CreateValueFromPpVarDictionary(pp::VarDictionary(var),
                                          error_message);
  }
  if (var.is_int()) return Value(var.AsInt());
  if (var.is_double()) return Value(var.AsDouble());
  if (var.is_array_buffer())
    return CreateValueFromVarArrayBuffer(pp::VarArrayBuffer(var));
  GOOGLE_SMART_CARD_NOTREACHED;
}

}  // namespace google_smart_card
