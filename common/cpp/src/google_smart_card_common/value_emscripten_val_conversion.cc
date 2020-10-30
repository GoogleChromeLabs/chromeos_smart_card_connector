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

#include <google_smart_card_common/value_emscripten_val_conversion.h>

#include <stdint.h>

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <emscripten/val.h>
#include <emscripten/wire.h>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

namespace {

constexpr char kErrorWrongType[] = "Conversion error: unsupported type \"%s\"";

emscripten::val CreateIntegerVal(int64_t integer) {
  // `emscripten::val` doesn't support direct conversion from `int64_t`, so
  // convert via `int` or `double` depending on the amount.
  if (std::numeric_limits<int>::min() <= integer &&
      integer <= std::numeric_limits<int>::max()) {
    return emscripten::val(static_cast<int>(integer));
  }
  // TODO(#217): Forbid conversions that lose precision.
  return emscripten::val(static_cast<double>(integer));
}

emscripten::val CreateArrayBufferVal(const Value::BinaryStorage& binary) {
  // Note: Not using conversions via `emscripten::memory_view`, since it'd be an
  // unowned pointer that won't be valid after `binary` is destroyed.
  const emscripten::val uint8_array =
      emscripten::val::global("Uint8Array")
          .call<emscripten::val>("from", emscripten::val::array(binary));
  return uint8_array["buffer"];
}

emscripten::val CreateObjectVal(const Value::DictionaryStorage& dictionary) {
  emscripten::val object = emscripten::val::object();
  for (const auto& item : dictionary) {
    const std::string& item_key = item.first;
    const std::unique_ptr<Value>& item_value = item.second;
    object.set(item_key, ConvertValueToEmscriptenVal(*item_value));
  }
  return object;
}

emscripten::val CreateArrayVal(const Value::ArrayStorage& array) {
  std::vector<emscripten::val> converted_items;
  for (const auto& item : array)
    converted_items.push_back(ConvertValueToEmscriptenVal(*item));
  return emscripten::val::array(converted_items);
}

bool IsEmcriptenValInstanceof(const emscripten::val& val,
                              const char* type_name) {
  // clang-format incorrectly adds a space after "instanceof".
  // clang-format off
  return val.instanceof(emscripten::val::global(type_name));
  // clang-format on
}

Value CreateValueFromNumberVal(const emscripten::val& val) {
  // Convert the number as `int`, in case it's an integer that fits into `int`.
  // Note that `emscripten::val` doesn't support direct conversions into
  // `int64_t`.
  // TODO(#217): Avoid conversions from imprecise numbers into integers.
  if (emscripten::val::global("Number").call<bool>("isInteger", val) &&
      val <= emscripten::val(std::numeric_limits<int>::max()) &&
      val >= emscripten::val(std::numeric_limits<int>::min()))
    return Value(val.as<int>());
  // The number is fractional or too big - therefore convert it into `double`.
  return Value(val.as<double>());
}

Value CreateValueFromArrayBufferVal(const emscripten::val& val) {
  const emscripten::val source_uint8_array =
      emscripten::val::global("Uint8Array").new_(val);
  std::vector<uint8_t> bytes(source_uint8_array["length"].as<int>());
  if (!bytes.empty()) {
    // Copy the array buffer in an effective way: create a typed array that
    // points to the `std::vector` allocated above, and fill it with contents of
    // `source_uint8_array` by a single call. This should be much faster than
    // accessing each byte one-by-one via operator[].
    const emscripten::val target_uint8_array = emscripten::val(
        emscripten::typed_memory_view(bytes.size(), bytes.data()));
    target_uint8_array.call<void>("set", source_uint8_array);
  }
  return Value(std::move(bytes));
}

optional<Value> CreateValueFromArrayLikeVal(const emscripten::val& val,
                                            std::string* error_message) {
  const size_t size = val["length"].as<size_t>();
  std::vector<std::unique_ptr<Value>> converted_items(size);
  std::string local_error_message;
  for (size_t index = 0; index < size; ++index) {
    optional<Value> converted_item =
        ConvertEmscriptenValToValue(val[index], &local_error_message);
    if (!converted_item) {
      FormatPrintfTemplateAndSet(
          error_message, "Error converting array item #%d: %s",
          static_cast<int>(index), local_error_message.c_str());
      return {};
    }
    converted_items[index] = MakeUnique<Value>(std::move(*converted_item));
  }
  return Value(std::move(converted_items));
}

optional<Value> CreateValueFromObjectVal(const emscripten::val& val,
                                         std::string* error_message) {
  const emscripten::val keys =
      emscripten::val::global("Object").call<emscripten::val>("keys", val);
  const size_t keys_size = keys["length"].as<size_t>();
  std::string local_error_message;
  Value value(Value::Type::kDictionary);
  for (size_t index = 0; index < keys_size; ++index) {
    const emscripten::val item_key = keys[index];
    const emscripten::val item_value = val[item_key];
    GOOGLE_SMART_CARD_CHECK(item_key.isString());
    const std::string key = item_key.as<std::string>();
    optional<Value> converted_item_value =
        ConvertEmscriptenValToValue(item_value, &local_error_message);
    if (!converted_item_value) {
      FormatPrintfTemplateAndSet(error_message,
                                 "Error converting object property \"%s\": %s",
                                 key.c_str(), local_error_message.c_str());
      return {};
    }
    value.SetDictionaryItem(key, std::move(*converted_item_value));
  }
  return std::move(value);
}

}  // namespace

emscripten::val ConvertValueToEmscriptenVal(const Value& value) {
  switch (value.type()) {
    case Value::Type::kNull:
      return emscripten::val::null();
    case Value::Type::kBoolean:
      return emscripten::val(value.GetBoolean());
    case Value::Type::kInteger:
      return CreateIntegerVal(value.GetInteger());
    case Value::Type::kFloat:
      return emscripten::val(value.GetFloat());
    case Value::Type::kString:
      return emscripten::val(value.GetString());
    case Value::Type::kBinary:
      return CreateArrayBufferVal(value.GetBinary());
    case Value::Type::kDictionary:
      return CreateObjectVal(value.GetDictionary());
    case Value::Type::kArray:
      return CreateArrayVal(value.GetArray());
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

optional<Value> ConvertEmscriptenValToValue(const emscripten::val& val,
                                            std::string* error_message) {
  if (val.isUndefined() || val.isNull()) return Value();
  if (val.isTrue()) return Value(true);
  if (val.isFalse()) return Value(false);
  if (val.isNumber()) return CreateValueFromNumberVal(val);
  if (val.isString()) return Value(val.as<std::string>());
  if (val.isArray()) return CreateValueFromArrayLikeVal(val, error_message);
  if (IsEmcriptenValInstanceof(val, "DataView")) {
    FormatPrintfTemplateAndSet(error_message, kErrorWrongType, "DataView");
    return {};
  }
  if (IsEmcriptenValInstanceof(val, "ArrayBuffer"))
    return CreateValueFromArrayBufferVal(val);
  if (emscripten::val::global("ArrayBuffer").call<bool>("isView", val)) {
    // Note that `ArrayBuffer.isView()` returns true for all TypedArray objects,
    // but also for `DataView` objects that aren't iterable and therefore have
    // to be excluded above.
    return CreateValueFromArrayLikeVal(val, error_message);
  }
  const std::string val_typeof = val.typeOf().as<std::string>();
  if (val_typeof == "object")
    return CreateValueFromObjectVal(val, error_message);
  // There's no easy way to stringify an arbitrary JavaScript value (e.g.,
  // calling "String()" might raise an exception), therefore simply use the
  // result of "typeof".
  FormatPrintfTemplateAndSet(error_message, kErrorWrongType,
                             val_typeof.c_str());
  return {};
}

Value ConvertEmscriptenValToValueOrDie(const emscripten::val& val) {
  std::string error_message;
  optional<Value> value = ConvertEmscriptenValToValue(val, &error_message);
  if (!value) GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  return std::move(*value);
}

}  // namespace google_smart_card
