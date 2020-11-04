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

#include <google_smart_card_common/pp_var_utils/copying.h>

#include <stdint.h>

#include <cstring>
#include <limits>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

namespace {

pp::Var CopyVarUpToDepth(const pp::Var& var, int depth);

pp::VarArray CopyVarArrayUpToDepth(const pp::VarArray& var, int depth) {
  GOOGLE_SMART_CARD_CHECK(depth > 0);
  pp::VarArray result;
  for (uint32_t index = 0; index < var.GetLength(); ++index) {
    const pp::Var result_item = CopyVarUpToDepth(var.Get(index), depth - 1);
    GOOGLE_SMART_CARD_CHECK(result.Set(index, result_item));
  }
  return result;
}

pp::VarArrayBuffer CopyVarArrayBuffer(pp::VarArrayBuffer var) {
  pp::VarArrayBuffer result(var.ByteLength());
  std::memcpy(result.Map(), var.Map(), var.ByteLength());
  var.Unmap();
  return result;
}

pp::VarDictionary CopyVarDictUpToDepth(const pp::VarDictionary& var,
                                       int depth) {
  GOOGLE_SMART_CARD_CHECK(depth > 0);
  const pp::VarArray keys = var.GetKeys();
  pp::VarDictionary result;
  for (uint32_t index = 0; index < keys.GetLength(); ++index) {
    const pp::Var key = keys.Get(index);
    const pp::Var result_value = CopyVarUpToDepth(var.Get(key), depth - 1);
    GOOGLE_SMART_CARD_CHECK(result.Set(key, result_value));
  }
  return result;
}

pp::Var CopyVarUpToDepth(const pp::Var& var, int depth) {
  GOOGLE_SMART_CARD_CHECK(depth >= 0);
  if (!depth)
    return var;
  if (var.is_undefined())
    return pp::Var();
  if (var.is_null())
    return pp::Var::Null();
  if (var.is_bool())
    return var.AsBool();
  if (var.is_string())
    return var.AsString();
  if (var.is_object())
    GOOGLE_SMART_CARD_LOG_FATAL << "Cannot copy object Pepper value";
  if (var.is_array())
    return CopyVarArrayUpToDepth(pp::VarArray(var), depth);
  if (var.is_dictionary())
    return CopyVarDictUpToDepth(pp::VarDictionary(var), depth);
  if (var.is_resource())
    GOOGLE_SMART_CARD_LOG_FATAL << "Cannot copy resource Pepper value";
  if (var.is_int())
    return var.AsInt();
  if (var.is_double())
    return var.AsDouble();
  if (var.is_array_buffer())
    return CopyVarArrayBuffer(pp::VarArrayBuffer(var));
  GOOGLE_SMART_CARD_NOTREACHED;
}

}  // namespace

pp::Var ShallowCopyVar(const pp::Var& var) {
  return CopyVarUpToDepth(var, 1);
}

pp::VarArray ShallowCopyVar(const pp::VarArray& var) {
  return CopyVarArrayUpToDepth(var, 1);
}

pp::VarArrayBuffer ShallowCopyVar(const pp::VarArrayBuffer& var) {
  return CopyVarArrayBuffer(var);
}

pp::VarDictionary ShallowCopyVar(const pp::VarDictionary& var) {
  return CopyVarDictUpToDepth(var, 1);
}

pp::Var DeepCopyVar(const pp::Var& var) {
  return CopyVarUpToDepth(var, std::numeric_limits<int>::max());
}

pp::VarArray DeepCopyVar(const pp::VarArray& var) {
  return CopyVarArrayUpToDepth(var, std::numeric_limits<int>::max());
}

pp::VarArrayBuffer DeepCopyVar(const pp::VarArrayBuffer& var) {
  return CopyVarArrayBuffer(var);
}

pp::VarDictionary DeepCopyVar(const pp::VarDictionary& var) {
  return CopyVarDictUpToDepth(var, std::numeric_limits<int>::max());
}

}  // namespace google_smart_card
