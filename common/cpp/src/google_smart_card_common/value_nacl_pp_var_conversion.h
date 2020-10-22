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

// Provides helper functions for converting between the `Value` and the Native
// Client's `pp::Var` classes.

#ifndef GOOGLE_SMART_CARD_COMMON_VALUE_NACL_PP_VAR_CONVERSION_H_
#define GOOGLE_SMART_CARD_COMMON_VALUE_NACL_PP_VAR_CONVERSION_H_

#include <string>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Converts the given `Value` into a `pp::Var`.
pp::Var ConvertValueToPpVar(const Value& value);

// Converts the given `pp::Var` into a `Value`.
//
// When the conversion isn't possible (e.g., when the passed variable contains a
// `pp::Resource` object), returns a null optional and, if provided, sets
// `*error_message`.
optional<Value> ConvertPpVarToValue(const pp::Var& var,
                                    std::string* error_message = nullptr);

// Same as `ConvertPpVarToValue()`, but immediately crashes the program if the
// conversion fails.
Value ConvertPpVarToValueOrDie(const pp::Var& var);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_NACL_PP_VAR_CONVERSION_H_
