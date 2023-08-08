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

// Helper functions for working with multi-strings.
//
// A multi-string is a sequence of zero-terminated strings with an additional
// zero character appended after the end of the last string.

#ifndef GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_MULTI_STRING_H_
#define GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_MULTI_STRING_H_

#include <string>
#include <vector>

namespace google_smart_card {

std::string CreateMultiString(const std::vector<std::string>& elements);

std::vector<std::string> ExtractMultiStringElements(
    const std::string& multi_string);

std::vector<std::string> ExtractMultiStringElements(const char* multi_string);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_MULTI_STRING_H_
