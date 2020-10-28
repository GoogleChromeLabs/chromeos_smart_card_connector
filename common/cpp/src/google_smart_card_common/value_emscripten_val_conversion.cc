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
#include <vector>

#include <emscripten/val.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

namespace {

emscripten::val CreateIntegerVal(int64_t integer) {
  // `emscripten::val` doesn't support direct conversion from `int64_t`, so
  // convert via `int` or `double` depending on the amount.
  if (std::numeric_limits<int>::min() <= integer &&
      integer <= std::numeric_limits<int>::max()) {
    return emscripten::val(static_cast<int>(integer));
  }
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

}  // namespace google_smart_card
