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
#include <string>
#include <utility>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

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
  }
  var_array_buffer.Unmap();
  return var_array_buffer;
}

pp::VarDictionary CreateVarDictionary(const Value& value) {
  GOOGLE_SMART_CARD_CHECK(value.is_dictionary());
  pp::VarDictionary var_dictionary;
  for (auto& item: value.GetDictionary()) {
    const std::string& item_key = item.first;
    const std::unique_ptr<Value>& item_value = item.second;
    var_dictionary.Set(item_key, ConvertValueToPpVar(*item_value));
  }
  return var_dictionary;
}

pp::VarArray CreateVarArray(const Value& value) {
  GOOGLE_SMART_CARD_CHECK(value.is_array());
  const Value::ArrayStorage& array_storage = value.GetArray();
  pp::VarArray var_array;
  var_array.SetLength(array_storage.size());
  for (size_t index = 0; index < array_storage.size(); ++index) {
    const std::unique_ptr<Value>& item = array_storage[index];
    GOOGLE_SMART_CARD_CHECK(var_array.Set(
        index, ConvertValueToPpVar(*item)));
  }
  return var_array;
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
      return CreateVarDictionary(value);
    case Value::Type::kArray:
      return CreateVarArray(value);
  }
  GOOGLE_SMART_CARD_NOTREACHED;
}

}  // namespace google_smart_card
