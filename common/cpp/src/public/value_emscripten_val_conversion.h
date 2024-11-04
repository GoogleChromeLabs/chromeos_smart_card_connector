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

// Provides helper functions for converting between the `Value` and the
// Emscripten's `emscripten::val` classes.

#ifndef GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_VALUE_EMSCRIPTEN_VAL_CONVERSION_H_
#define GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_VALUE_EMSCRIPTEN_VAL_CONVERSION_H_

#ifndef __EMSCRIPTEN__
#error "This file should only be used in Emscripten builds"
#endif  // __EMSCRIPTEN__

#include <optional>
#include <string>

#include <emscripten/val.h>

#include "common/cpp/src/public/value.h"

namespace google_smart_card {

// Converts the given `Value` into a `emscripten::val`.
//
// When the conversion isn't possible (e.g., when the passed value is a huge
// integer that can't be precisely represented as a JavaScript number), returns
// a null optional and, if provided, sets `*error_message`.
std::optional<emscripten::val> ConvertValueToEmscriptenVal(
    const Value& value,
    std::string* error_message = nullptr);

// Same as `ConvertValueToEmscriptenVal()`, but immediately crashes the program
// if the conversion fails.
emscripten::val ConvertValueToEmscriptenValOrDie(const Value& value);

// Converts the given `emscripten::val` into a `Value`.
//
// When the conversion isn't possible (e.g., when the passed variable is a
// function), returns a null optional and, if provided, sets `*error_message`.
std::optional<Value> ConvertEmscriptenValToValue(
    const emscripten::val& val,
    std::string* error_message = nullptr);

// Same as `ConvertEmscriptenValToValue()`, but immediately crashes the program
// if the conversion fails.
Value ConvertEmscriptenValToValueOrDie(const emscripten::val& val);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_VALUE_EMSCRIPTEN_VAL_CONVERSION_H_
