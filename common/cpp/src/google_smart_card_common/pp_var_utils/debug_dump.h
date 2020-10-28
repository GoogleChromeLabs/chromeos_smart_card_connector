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

// This file contains helper functions and constants for obtaining a debug
// information for the Pepper values.

#ifndef GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_DEBUG_DUMP_H_
#define GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_DEBUG_DUMP_H_

#ifndef __native_client__
#error "This file should only be used in Native Client builds"
#endif  // __native_client__

#include <string>

#include <ppapi/cpp/var.h>

namespace google_smart_card {

//
// The set of constants for the Pepper value type description.
//
// Note that both JavaScript and Pepper have their own type system, and neither
// of them provides a really practical classification (e.g. "typeof null"
// expression in JavaScript equals to "object", while in Pepper API, for
// example, both strings and numbers are stored in objects of the same pp::Var
// class). That's why our "title" here is a non-standard entity whose main aim
// is to provide a useful debug representation of the Pepper values.
//
extern const char kUndefinedJsTypeTitle[];
extern const char kNullJsTypeTitle[];
extern const char kBooleanJsTypeTitle[];
extern const char kStringJsTypeTitle[];
extern const char kObjectJsTypeTitle[];
extern const char kArrayJsTypeTitle[];
extern const char kDictionaryJsTypeTitle[];
extern const char kResourceJsTypeTitle[];
extern const char kIntegerJsTypeTitle[];
extern const char kRealJsTypeTitle[];
extern const char kArrayBufferJsTypeTitle[];

// Returns the type title for the given Pepper value.
//
// The returned value will be one of the "*Title" constants listed above.
std::string GetVarTypeTitle(const pp::Var& var);

// Generates a debug representation of the given Pepper value in the Debug
// builds.
//
// In Release builds, always returns just the value type title.
//
// Note: for privacy reasons, any user-sensitive data must be passed through
// this function before being printed in logs, error messages, etc.
std::string DebugDumpVar(const pp::Var& var);

// Generates a debug representation of the given Pepper value.
//
// Note that this function always performs the full dump, even in the Release
// builds - so for privacy reasons for any user-sensitive data the
// DebugDumpVar() function should be used instead.
std::string DumpVar(const pp::Var& var);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_DEBUG_DUMP_H_
