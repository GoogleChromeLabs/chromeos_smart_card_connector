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

#include <google_smart_card_common/pp_var_utils/extraction.h>

#include <cstring>

#include <google_smart_card_common/numeric_conversions.h>

namespace google_smart_card {

namespace {

constexpr char kErrorWrongType[] =
    "Expected a value of type \"%s\", instead got: %s";

template <typename T>
bool VarAsInteger(const pp::Var& var,
                  const char* type_name,
                  T* result,
                  std::string* error_message) {
  int64_t integer;
  if (var.is_int()) {
    integer = var.AsInt();
  } else if (var.is_double()) {
    if (!CastDoubleToInt64(var.AsDouble(), &integer, error_message))
      return false;
  } else {
    *error_message = FormatPrintfTemplate(kErrorWrongType, kIntegerJsTypeTitle,
                                          DebugDumpVar(var).c_str());
    return false;
  }
  return CastInteger(integer, type_name, result, error_message);
}

}  // namespace

bool VarAs(const pp::Var& var, int8_t* result, std::string* error_message) {
  return VarAsInteger(var, "int8_t", result, error_message);
}

bool VarAs(const pp::Var& var, uint8_t* result, std::string* error_message) {
  return VarAsInteger(var, "uint8_t", result, error_message);
}

bool VarAs(const pp::Var& var, int16_t* result, std::string* error_message) {
  return VarAsInteger(var, "int16_t", result, error_message);
}

bool VarAs(const pp::Var& var, uint16_t* result, std::string* error_message) {
  return VarAsInteger(var, "uint16_t", result, error_message);
}

bool VarAs(const pp::Var& var, int32_t* result, std::string* error_message) {
  return VarAsInteger(var, "int32_t", result, error_message);
}

bool VarAs(const pp::Var& var, uint32_t* result, std::string* error_message) {
  return VarAsInteger(var, "uint32_t", result, error_message);
}

bool VarAs(const pp::Var& var, int64_t* result, std::string* error_message) {
  return VarAsInteger(var, "int64_t", result, error_message);
}

bool VarAs(const pp::Var& var, uint64_t* result, std::string* error_message) {
  return VarAsInteger(var, "uint64_t", result, error_message);
}

bool VarAs(const pp::Var& var, long* result, std::string* error_message) {
  return VarAsInteger(var, "long", result, error_message);
}

bool VarAs(const pp::Var& var,
           unsigned long* result,
           std::string* error_message) {
  return VarAsInteger(var, "unsigned long", result, error_message);
}

bool VarAs(const pp::Var& var, float* result, std::string* error_message) {
  double double_value;
  if (!VarAs(var, &double_value, error_message))
    return false;
  *result = static_cast<float>(double_value);
  return true;
}

bool VarAs(const pp::Var& var, double* result, std::string* error_message) {
  if (!var.is_number()) {
    *error_message = FormatPrintfTemplate(kErrorWrongType, kIntegerJsTypeTitle,
                                          DebugDumpVar(var).c_str());
    return false;
  }
  *result = var.AsDouble();
  return true;
}

bool VarAs(const pp::Var& var, bool* result, std::string* error_message) {
  if (!var.is_bool()) {
    *error_message = FormatPrintfTemplate(kErrorWrongType, kBooleanJsTypeTitle,
                                          DebugDumpVar(var).c_str());
    return false;
  }
  *result = var.AsBool();
  return true;
}

bool VarAs(const pp::Var& var,
           std::string* result,
           std::string* error_message) {
  if (!var.is_string()) {
    *error_message = FormatPrintfTemplate(kErrorWrongType, kStringJsTypeTitle,
                                          DebugDumpVar(var).c_str());
    return false;
  }
  *result = var.AsString();
  return true;
}

bool VarAs(const pp::Var& var,
           pp::Var* result,
           std::string* /*error_message*/) {
  *result = var;
  return true;
}

bool VarAs(const pp::Var& var,
           pp::VarArray* result,
           std::string* error_message) {
  if (!var.is_array()) {
    *error_message = FormatPrintfTemplate(kErrorWrongType, kArrayJsTypeTitle,
                                          DebugDumpVar(var).c_str());
    return false;
  }
  *result = pp::VarArray(var);
  return true;
}

bool VarAs(const pp::Var& var,
           pp::VarArrayBuffer* result,
           std::string* error_message) {
  if (!var.is_array_buffer()) {
    *error_message = FormatPrintfTemplate(
        kErrorWrongType, kArrayBufferJsTypeTitle, DebugDumpVar(var).c_str());
    return false;
  }
  *result = pp::VarArrayBuffer(var);
  return true;
}

bool VarAs(const pp::Var& var,
           pp::VarDictionary* result,
           std::string* error_message) {
  if (!var.is_dictionary()) {
    *error_message = FormatPrintfTemplate(
        kErrorWrongType, kDictionaryJsTypeTitle, DebugDumpVar(var).c_str());
    return false;
  }
  *result = pp::VarDictionary(var);
  return true;
}

bool VarAs(const pp::Var& var,
           pp::Var::Null* /*result*/,
           std::string* error_message) {
  if (!var.is_null()) {
    *error_message = FormatPrintfTemplate(kErrorWrongType, kNullJsTypeTitle,
                                          DebugDumpVar(var).c_str());
    return false;
  }
  return true;
}

namespace internal {

std::vector<uint8_t> GetVarArrayBufferData(pp::VarArrayBuffer var) {
  std::vector<uint8_t> result(var.ByteLength());
  if (!result.empty()) {
    std::memcpy(&result[0], var.Map(), result.size());
    var.Unmap();
  }
  return result;
}

}  // namespace internal

int GetVarDictSize(const pp::VarDictionary& var) {
  return GetVarArraySize(var.GetKeys());
}

int GetVarArraySize(const pp::VarArray& var) {
  return static_cast<int>(var.GetLength());
}

bool GetVarDictValue(const pp::VarDictionary& var,
                     const std::string& key,
                     pp::Var* result,
                     std::string* error_message) {
  if (!var.HasKey(key)) {
    *error_message =
        FormatPrintfTemplate("The dictionary has no key \"%s\"", key.c_str());
    return false;
  }
  *result = var.Get(key);
  return true;
}

pp::Var GetVarDictValue(const pp::VarDictionary& var, const std::string& key) {
  pp::Var result;
  std::string error_message;
  if (!GetVarDictValue(var, key, &result, &error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  return result;
}

VarDictValuesExtractor::VarDictValuesExtractor(const pp::VarDictionary& var)
    : var_(var) {
  const std::vector<std::string> keys =
      VarAs<std::vector<std::string>>(var_.GetKeys());
  not_requested_keys_.insert(keys.begin(), keys.end());
}

bool VarDictValuesExtractor::GetSuccess(std::string* error_message) const {
  if (failed_) {
    *error_message = error_message_;
    return false;
  }
  return true;
}

bool VarDictValuesExtractor::GetSuccessWithNoExtraKeysAllowed(
    std::string* error_message) const {
  if (!GetSuccess(error_message))
    return false;
  if (!not_requested_keys_.empty()) {
    std::string unexpected_keys_dump;
    for (const std::string& key : not_requested_keys_) {
      if (!unexpected_keys_dump.empty())
        unexpected_keys_dump += ", ";
      unexpected_keys_dump += '"' + key + '"';
    }
    *error_message =
        FormatPrintfTemplate("The dictionary contains unexpected keys: %s",
                             unexpected_keys_dump.c_str());
    return false;
  }
  return true;
}

void VarDictValuesExtractor::CheckSuccess() const {
  std::string error_message;
  if (!GetSuccess(&error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
}

void VarDictValuesExtractor::CheckSuccessWithNoExtraKeysAllowed() const {
  std::string error_message;
  if (!GetSuccessWithNoExtraKeysAllowed(&error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
}

void VarDictValuesExtractor::AddRequestedKey(const std::string& key) {
  not_requested_keys_.erase(key);
}

void VarDictValuesExtractor::ProcessFailedExtraction(
    const std::string& key,
    const std::string& extraction_error_message) {
  if (failed_) {
    // We could concatenate all occurred errors, but storing of the first error
    // only should be enough.
    return;
  }
  error_message_ = FormatPrintfTemplate(
      "Failed to extract the dictionary value with key \"%s\": %s", key.c_str(),
      extraction_error_message.c_str());
  failed_ = true;
}

}  // namespace google_smart_card
