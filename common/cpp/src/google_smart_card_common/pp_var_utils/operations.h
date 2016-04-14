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

// This file contains functions implementing various transformation operations
// with the Pepper values.

#ifndef GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_OPERATIONS_H_
#define GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_OPERATIONS_H_

#include <stdint.h>

#include <string>

#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/construction.h>

namespace google_smart_card {

// Sets the Pepper array item.
//
// Apart from asserting the operation success, this function performs automatic
// conversion of the passed value into the Pepper value (using the MakeVar
// function in construction.h file).
template <typename T>
inline void SetVarArrayItem(pp::VarArray* var, size_t index, const T& value) {
  GOOGLE_SMART_CARD_CHECK(var->Set(index, MakeVar(value)));
}

// Returns a sub-array of the given Pepper array.
//
// Asserts the validity of the specified indices.
pp::VarArray SliceVarArray(
  const pp::VarArray& var, uint32_t begin_index, uint32_t count);

// Adds or updates the Pepper dictionary item.
//
// Apart from asserting the operation success, this function performs automatic
// conversion of the passed value into the Pepper value (using the MakeVar
// function in construction.h file).
template <typename T>
inline void SetVarDictValue(
    pp::VarDictionary* var, const std::string& key, const T& value) {
  GOOGLE_SMART_CARD_CHECK(var->Set(key, MakeVar(value)));
}

// Adds an item to the Pepper dictionary, asserting that the key inserted didn't
// exist previously.
//
// Apart from asserting the operation success, this function performs automatic
// conversion of the passed value into the Pepper value (using the MakeVar
// function in construction.h file).
template <typename T>
inline void AddVarDictValue(
    pp::VarDictionary* var, const std::string& key, const T& value) {
  GOOGLE_SMART_CARD_CHECK(!var->HasKey(key));
  SetVarDictValue(var, key, value);
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_OPERATIONS_H_
