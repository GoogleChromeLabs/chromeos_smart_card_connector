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

// This file contains helper functions for dumping bit masks.

#ifndef GOOGLE_SMART_CARD_COMMON_LOGGING_MASK_DUMP_H_
#define GOOGLE_SMART_CARD_COMMON_LOGGING_MASK_DUMP_H_

#include <string>
#include <vector>

#include <google_smart_card_common/logging/hex_dumping.h>

namespace google_smart_card {

template <typename T>
struct MaskOptionValueWithName {
  MaskOptionValueWithName(T value, const std::string& name)
      : value(value), name(name) {}

  T value;
  std::string name;
};

template <typename T, typename... Args>
inline std::string DumpMask(
    T value, const std::vector<MaskOptionValueWithName<T>>& options) {
  std::string result;
  for (const auto& option : options) {
    if (!(value & option.value)) continue;
    if (!result.empty()) result += '|';
    result += option.name;
    value &= ~option.value;
  }
  if (value) {
    if (!result.empty()) result += '|';
    result += HexDumpInteger(value);
  }
  if (result.empty()) return "0";
  return result;
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_LOGGING_MASK_DUMP_H_
