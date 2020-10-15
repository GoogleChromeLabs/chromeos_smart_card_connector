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

#ifndef GOOGLE_SMART_CARD_COMMON_VALUE_DEBUG_DUMPING_H_
#define GOOGLE_SMART_CARD_COMMON_VALUE_DEBUG_DUMPING_H_

#include <string>

#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Generates a sanitized debug representation of the given value. In Release
// builds, this only dumps the title of the value's type; in Debug builds, this
// is equivalent to `DebugDumpValueFull()`.
std::string DebugDumpValueSanitized(const Value& value);

// Generates a full debug representation of the given value.
//
// NOTE: It's dangerous to use this function with variables that might
// potentially contain privacy/security sensitive data. Use
// `DebugDumpValueSanitized()` instead.
std::string DebugDumpValueFull(const Value& value);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_DEBUG_DUMPING_H_
