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

#include <google_smart_card_common/pp_var_utils/debug_dump.h>

#include <stdint.h>

#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/hex_dumping.h>
#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

// Definitions of the constants declared in the header file
const char kUndefinedJsTypeTitle[] = "undefined";
const char kNullJsTypeTitle[] = "null";
const char kBooleanJsTypeTitle[] = "Boolean";
const char kStringJsTypeTitle[] = "String";
const char kObjectJsTypeTitle[] = "Object";
const char kArrayJsTypeTitle[] = "Array";
const char kDictionaryJsTypeTitle[] = "Dictionary";
const char kResourceJsTypeTitle[] = "Resource";
const char kIntegerJsTypeTitle[] = "Integer";
const char kRealJsTypeTitle[] = "Real";
const char kArrayBufferJsTypeTitle[] = "ArrayBuffer";

std::string GetVarTypeTitle(const pp::Var& var) {
  if (var.is_undefined())
    return kUndefinedJsTypeTitle;
  if (var.is_null())
    return kNullJsTypeTitle;
  if (var.is_bool())
    return kBooleanJsTypeTitle;
  if (var.is_string())
    return kStringJsTypeTitle;
  if (var.is_object())
    return kObjectJsTypeTitle;
  if (var.is_array())
    return kArrayJsTypeTitle;
  if (var.is_dictionary())
    return kDictionaryJsTypeTitle;
  if (var.is_resource())
    return kResourceJsTypeTitle;
  if (var.is_int())
    return kIntegerJsTypeTitle;
  if (var.is_double())
    return kRealJsTypeTitle;
  if (var.is_array_buffer())
    return kArrayBufferJsTypeTitle;
  GOOGLE_SMART_CARD_NOTREACHED;
}

namespace {

std::string DumpBoolValue(bool value) {
  return value ? "true" : "false";
}

std::string DumpStringValue(const std::string& value) {
  return '"' + value + '"';
}

std::string DumpVarArrayValue(const pp::VarArray& var) {
  std::string result = "[";
  for (uint32_t index = 0; index < var.GetLength(); ++index) {
    if (index > 0)
      result += ", ";
    result += DumpVar(var.Get(index));
  }
  result += "]";
  return result;
}

std::string DumpVarDictValue(const pp::VarDictionary& var) {
  std::string result = "{";
  const pp::VarArray keys = var.GetKeys();
  for (uint32_t index = 0; index < keys.GetLength(); ++index) {
    if (index > 0)
      result += ", ";
    const pp::Var key = keys.Get(index);
    GOOGLE_SMART_CARD_CHECK(key.is_string());
    result += DumpStringValue(key.AsString()) + ": " + DumpVar(var.Get(key));
  }
  result += "}";
  return result;
}

std::string DumpVarArrayBufferValue(pp::VarArrayBuffer var) {
  const uint8_t* const data = static_cast<const uint8_t*>(var.Map());
  std::string result = std::string(kArrayBufferJsTypeTitle) + "[";
  for (uint32_t offset = 0; offset < var.ByteLength(); ++offset) {
    if (offset > 0)
      result += ", ";
    result += HexDumpByte(data[offset]);
  }
  result += "]";
  var.Unmap();
  return result;
}

}  // namespace

std::string DebugDumpVar(const pp::Var& var) {
#ifdef NDEBUG
  return GetVarTypeTitle(var);
#else
  return DumpVar(var);
#endif
}

std::string DumpVar(const pp::Var& var) {
  if (var.is_undefined())
    return kUndefinedJsTypeTitle;
  if (var.is_null())
    return kNullJsTypeTitle;
  if (var.is_bool())
    return DumpBoolValue(var.AsBool());
  if (var.is_string())
    return DumpStringValue(var.AsString());
  if (var.is_object())
    return std::string(kObjectJsTypeTitle) + "<...>";
  if (var.is_array())
    return DumpVarArrayValue(pp::VarArray(var));
  if (var.is_dictionary())
    return DumpVarDictValue(pp::VarDictionary(var));
  if (var.is_resource())
    return std::string(kResourceJsTypeTitle) + "<...>";
  if (var.is_int())
    return HexDumpUnknownSizeInteger(static_cast<int64_t>(var.AsInt()));
  if (var.is_double())
    return std::to_string(var.AsDouble());
  if (var.is_array_buffer())
    return DumpVarArrayBufferValue(pp::VarArrayBuffer(var));
  GOOGLE_SMART_CARD_NOTREACHED;
}

}  // namespace google_smart_card
