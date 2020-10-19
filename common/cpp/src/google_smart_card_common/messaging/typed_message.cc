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

#include <google_smart_card_common/messaging/typed_message.h>

#include <string>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/pp_var_utils/struct_converter.h>

namespace google_smart_card {

// static
template <>
constexpr const char* StructConverter<TypedMessage>::GetStructTypeName() {
  return "TypedMessage";
}

// static
template <>
template <typename Callback>
void StructConverter<TypedMessage>::VisitFields(const TypedMessage& value,
                                                Callback callback) {
  callback(&value.type, "type");
  callback(&value.data, "data");
}

bool VarAs(const pp::Var& var, TypedMessage* result,
           std::string* error_message) {
  return StructConverter<TypedMessage>::ConvertFromVar(var, result,
                                                       error_message);
}

pp::Var MakeVar(const TypedMessage& value) {
  return StructConverter<TypedMessage>::ConvertToVar(value);
}

}  // namespace google_smart_card
