// Copyright 2022 Google Inc. All Rights Reserved.
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

#ifndef GOOGLE_SMART_CARD_COMMON_JOIN_STRING_H_
#define GOOGLE_SMART_CARD_COMMON_JOIN_STRING_H_

#include <string>

namespace google_smart_card {

template <typename Container>
std::string JoinStrings(const Container& container,
                        const std::string& separator) {
  if (container.empty()) {
    return "";
  }

  size_t need_size = separator.size() * (container.size() - 1);
  for (const auto& item : container) {
    need_size += item.length();
  }

  std::string joined;
  joined.reserve(need_size);
  bool is_first = true;
  for (const auto& item : container) {
    if (!is_first) {
      joined += separator;
    }
    is_first = false;
    joined += item;
  }
  return joined;
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_JOIN_STRING_H_
