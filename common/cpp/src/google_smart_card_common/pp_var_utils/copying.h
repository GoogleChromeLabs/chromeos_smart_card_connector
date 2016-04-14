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

// This file contains functions for creating the deep and shallow copies of
// Pepper values.
//
// Note that using these functions is crucial in some situations, as the Pepper
// values themselves are ref-counted, and the pp::Var copy constructor only
// creates an object pointing to the same ref-counted base. As there is no
// copy-on-write behavior for Pepper values, when changing a mutable Pepper
// value (array or dictionary) one might need to copy it.

#ifndef GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_COPYING_H_
#define GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_COPYING_H_

#include <stdint.h>

#include <string>
#include <vector>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

//
// Creates a shallow copy of the given Pepper value: the result is a new Pepper
// value separate from the old one, but with all sub-values (array or dictionary
// items) kept pointing to the original sub-values.
//

pp::Var ShallowCopyVar(const pp::Var& var);

pp::VarArray ShallowCopyVar(const pp::VarArray& var);

pp::VarArrayBuffer ShallowCopyVar(const pp::VarArrayBuffer& var);

pp::VarDictionary ShallowCopyVar(const pp::VarDictionary& var);

//
// Creates a deep copy of the given Pepper value: the result is a new Pepper
// value completely separate from the old one, including all sub-values (array
// or dictionary items) recursively.
//

pp::Var DeepCopyVar(const pp::Var& var);

pp::VarArray DeepCopyVar(const pp::VarArray& var);

pp::VarArrayBuffer DeepCopyVar(const pp::VarArrayBuffer& var);

pp::VarDictionary DeepCopyVar(const pp::VarDictionary& var);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_COPYING_H_
