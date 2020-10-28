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

// This file contains various helper functions for extracting the values from
// the Pepper values (pp::Var type and its descendant types).
//
// Basically, each kind of conversion here is present in two versions: the one
// is returning the success along with an error message, and the other is
// crashing if the conversion failed.

#ifndef GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_EXTRACTION_H_
#define GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_EXTRACTION_H_

#ifndef __native_client__
#error "This file should only be used in Native Client builds"
#endif  // __native_client__

#include <inttypes.h>
#include <stdint.h>

#include <set>
#include <string>
#include <vector>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>

namespace google_smart_card {

//
// Following there is a group of functions that extract the value of the
// specific type from the generic Pepper variable (pp::Var type).
//
// This provides a uniform interface for performing such conversions (comparing
// to the somewhat discordant API offered by the pp::Var).
//
// The failure (by returning false and setting error_message) happens if the
// actual value stored in pp::Var is of completely incompatible type or if it is
// outside the representation range of the requested type (for the numeric data
// types).
//
// Note that the consumers may provide additional overloads for supporting the
// custom types; this would automatically add a support for them into the other
// functions defined in this file.
//

bool VarAs(const pp::Var& var, int8_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, uint8_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, int16_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, uint16_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, int32_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, uint32_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, int64_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, uint64_t* result, std::string* error_message);

bool VarAs(const pp::Var& var, long* result, std::string* error_message);

bool VarAs(const pp::Var& var, unsigned long* result,
           std::string* error_message);

bool VarAs(const pp::Var& var, float* result, std::string* error_message);

bool VarAs(const pp::Var& var, double* result, std::string* error_message);

bool VarAs(const pp::Var& var, bool* result, std::string* error_message);

bool VarAs(const pp::Var& var, std::string* result, std::string* error_message);

bool VarAs(const pp::Var& var, pp::Var* result, std::string* error_message);

bool VarAs(const pp::Var& var, pp::VarArray* result,
           std::string* error_message);

bool VarAs(const pp::Var& var, pp::VarArrayBuffer* result,
           std::string* error_message);

bool VarAs(const pp::Var& var, pp::VarDictionary* result,
           std::string* error_message);

bool VarAs(const pp::Var& var, pp::Var::Null* result,
           std::string* error_message);

template <typename T>
inline bool VarAs(const pp::Var& var, optional<T>* result,
                  std::string* error_message) {
  if (var.is_undefined() || var.is_null()) {
    result->reset();
    return true;
  }
  T result_value;
  if (!VarAs(var, &result_value, error_message)) return false;
  *result = result_value;
  return true;
}

namespace internal {

template <typename T>
inline bool GetVarArrayItemsVector(const pp::VarArray& var,
                                   std::vector<T>* result,
                                   std::string* error_message) {
  result->resize(var.GetLength());
  for (uint32_t index = 0; index < var.GetLength(); ++index) {
    if (!VarAs(var.Get(index), &(*result)[index], error_message)) {
      *error_message = FormatPrintfTemplate(
          "Failed to extract the array item with index %" PRIu32 ": %s", index,
          error_message->c_str());
      return false;
    }
  }
  return true;
}

template <typename T>
inline bool VarAsVarArrayAsVector(const pp::Var& var, std::vector<T>* result,
                                  std::string* error_message) {
  pp::VarArray var_array;
  if (!VarAs(var, &var_array, error_message)) return false;
  return GetVarArrayItemsVector(var_array, result, error_message);
}

std::vector<uint8_t> GetVarArrayBufferData(pp::VarArrayBuffer var);

inline bool VarAsVarArrayBufferAsUint8Vector(const pp::Var& var,
                                             std::vector<uint8_t>* result,
                                             std::string* error_message) {
  pp::VarArrayBuffer var_array_buffer;
  if (!VarAs(var, &var_array_buffer, error_message)) {
    *error_message = FormatPrintfTemplate(
        "Expected an array of unsigned bytes or ArrayBuffer, instead got: %s",
        DebugDumpVar(var).c_str());
    return false;
  }
  *result = GetVarArrayBufferData(var_array_buffer);
  return true;
}

}  // namespace internal

template <typename T>
inline bool VarAs(const pp::Var& var, std::vector<T>* result,
                  std::string* error_message) {
  return internal::VarAsVarArrayAsVector(var, result, error_message);
}

template <>
inline bool VarAs(const pp::Var& var, std::vector<uint8_t>* result,
                  std::string* error_message) {
  return internal::VarAsVarArrayAsVector(var, result, error_message) ||
         internal::VarAsVarArrayBufferAsUint8Vector(var, result, error_message);
}

// Extracts the value of given type from the generic Pepper variable (pp::Var).
//
// Asserts that the actual value stored in pp::Var has the compatible type and
// is inside the representation range of the requested type.
template <typename T>
inline T VarAs(const pp::Var& var) {
  T result;
  std::string error_message;
  if (!VarAs(var, &result, &error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  return result;
}

// Returns the number of items in the Pepper dictionary.
int GetVarDictSize(const pp::VarDictionary& var);

// Returns the number of items in the Pepper array.
int GetVarArraySize(const pp::VarArray& var);

// Extracts the value from the Pepper dictionary by the given key.
//
// Returns false (and sets error_message) if the requested key is not present.
bool GetVarDictValue(const pp::VarDictionary& var, const std::string& key,
                     pp::Var* result, std::string* error_message);

// Extracts the value from the dictionary by the given key.
//
// Asserts that the requested key is present.
pp::Var GetVarDictValue(const pp::VarDictionary& var, const std::string& key);

// Extracts the value of given type from the Pepper dictionary by the given key.
//
// The actual value conversion is done through one of the VarAs function
// overloads.
//
// Returns false (and sets error_message) if the requested key is not present or
// if the value conversion didn't succeed.
template <typename T>
inline bool GetVarDictValueAs(const pp::VarDictionary& var,
                              const std::string& key, T* result,
                              std::string* error_message) {
  pp::Var value_var;
  if (!GetVarDictValue(var, key, &value_var, error_message)) {
    *error_message = FormatPrintfTemplate(
        "Failed to extract the dictionary value with key \"%s\": %s",
        key.c_str(), error_message->c_str());
    return false;
  }
  return VarAs(value_var, result, error_message);
}

// Extracts the value of given type from the Pepper dictionary by the given key.
//
// The actual value conversion is done through one of the VarAs function
// overloads.
//
// Asserts that the request key is present and that the value conversion
// succeeded.
template <typename T>
inline T GetVarDictValueAs(const pp::VarDictionary& var,
                           const std::string& key) {
  T result;
  std::string error_message;
  if (!GetVarDictValueAs(var, key, &result, &error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  return result;
}

namespace internal {

inline bool IsVarArraySizeValid(const pp::VarArray& var, size_t expected_size,
                                std::string* error_message) {
  if (var.GetLength() != expected_size) {
    *error_message = FormatPrintfTemplate(
        "Expected an array of size %zu, instead got: %" PRIu32, expected_size,
        var.GetLength());
    return false;
  }
  return true;
}

}  // namespace internal

inline bool TryGetVarArrayItemsInternal(const pp::VarArray& /*var*/,
                                        uint32_t /*current_item_index*/,
                                        std::string* /*error_message*/) {
  return true;
}

template <typename Arg, typename... Args>
inline bool TryGetVarArrayItemsInternal(const pp::VarArray& var,
                                        uint32_t current_item_index,
                                        std::string* error_message, Arg* arg,
                                        Args*... args) {
  if (!VarAs(var.Get(current_item_index), arg, error_message)) {
    *error_message = FormatPrintfTemplate(
        "Failed to extract the array item with index %" PRIu32 ": %s",
        current_item_index, error_message->c_str());
    return false;
  }
  return TryGetVarArrayItemsInternal(var, current_item_index + 1, error_message,
                                     args...);
}

// Extracts the items of the Pepper array into the passed value sequence.
//
// Returns false (and sets error_message) if the array has a different length or
// if some value conversion didn't succeed.
template <typename... Args>
inline bool TryGetVarArrayItems(const pp::VarArray& var,
                                std::string* error_message, Args*... args) {
  return internal::IsVarArraySizeValid(var, sizeof...(Args), error_message) &&
         TryGetVarArrayItemsInternal(var, 0, error_message, args...);
}

// Extracts the items of the Pepper array into the passed value sequence.
//
// Asserts that the array has the desired length and that all values conversion
// succeeded.
template <typename... Args>
inline void GetVarArrayItems(const pp::VarArray& var, Args*... args) {
  std::string error_message;
  if (!TryGetVarArrayItems(var, &error_message, args...))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
}

// Extracts the items of the given Pepper dictionary value, assuming that all of
// the keys are known in advance.
//
// The extraction fails if some of the required keys are missing, or if some of
// the values conversion failed.
//
// Optionally, a check that no additional keys are present apart from the
// requested ones can be issued.
//
// A typical usage example:
//    VarDictValuesExtractor(var_dictionary)
//        .Extract("key_1", &value_1)
//        .Extract("key_2", &value_2)
//        .TryExtractOptional("optional_key_3", &value_3)
//        .GetSuccess(&error_message);
class VarDictValuesExtractor final {
 public:
  explicit VarDictValuesExtractor(const pp::VarDictionary& var);
  VarDictValuesExtractor(const VarDictValuesExtractor&) = delete;

  // Extracts a dictionary value by the specified key. The key must exist in the
  // dictionary.
  //
  // Returns *this to simplify coding by allowing chaining of multiple calls.
  template <typename T>
  VarDictValuesExtractor& Extract(const std::string& key, T* result) {
    std::string extraction_error_message;
    if (!GetVarDictValueAs(var_, key, result, &extraction_error_message))
      ProcessFailedExtraction(key, extraction_error_message);
    AddRequestedKey(key);
    return *this;
  }

  // Extracts a dictionary value by the specified key. If the key is missing,
  // sets the resulting value to an empty optional.
  //
  // Returns *this to simplify coding by allowing chaining of multiple calls.
  template <typename T>
  VarDictValuesExtractor& TryExtractOptional(const std::string& key,
                                             optional<T>* result) {
    std::string extraction_error_message;
    pp::Var var_value;
    if (!GetVarDictValue(var_, key, &var_value, &extraction_error_message)) {
      result->reset();
    } else {
      *result = T();
      if (!VarAs(var_value, &result->value(), &extraction_error_message))
        ProcessFailedExtraction(key, extraction_error_message);
    }
    AddRequestedKey(key);
    return *this;
  }

  // Returns whether all extraction requests succeeded.
  bool GetSuccess(std::string* error_message) const;

  // Returns whether all extraction requests succeeded, and no other keys are
  // present in the dictionary.
  bool GetSuccessWithNoExtraKeysAllowed(std::string* error_message) const;

  // Asserts that GetSuccess method returns true.
  void CheckSuccess() const;

  // Asserts that GetSuccess method returns true.
  void CheckSuccessWithNoExtraKeysAllowed() const;

 private:
  void AddRequestedKey(const std::string& key);

  void ProcessFailedExtraction(const std::string& key,
                               const std::string& extraction_error_message);

  const pp::VarDictionary var_;
  std::set<std::string> not_requested_keys_;
  bool failed_ = false;
  std::string error_message_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_EXTRACTION_H_
