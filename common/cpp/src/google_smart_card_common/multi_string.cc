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

#include <google_smart_card_common/multi_string.h>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

namespace {

std::vector<std::string> ExtractMultiStringElements(
    const char* multi_string,
    const char** multi_string_end) {
  std::vector<std::string> result;
  const char* current_begin = multi_string;
  while (*current_begin != '\0') {
    const std::string current_string(current_begin);
    result.push_back(current_string);
    current_begin += current_string.length() + 1;
  }
  *multi_string_end = current_begin + 1;
  return result;
}

}  // namespace

std::string CreateMultiString(const std::vector<std::string>& elements) {
  std::string result;
  for (const auto& element : elements) {
    GOOGLE_SMART_CARD_CHECK(element.find('\0') == std::string::npos);
    result += element;
    result += '\0';
  }
  result += '\0';
  return result;
}

std::vector<std::string> ExtractMultiStringElements(
    const std::string& multi_string) {
  GOOGLE_SMART_CARD_CHECK(multi_string.length());
  GOOGLE_SMART_CARD_CHECK(multi_string.back() == '\0');
  const char* multi_string_end;
  const std::vector<std::string> result =
      ExtractMultiStringElements(multi_string.c_str(), &multi_string_end);
  GOOGLE_SMART_CARD_CHECK(multi_string_end ==
                          multi_string.c_str() + multi_string.length());
  return result;
}

std::vector<std::string> ExtractMultiStringElements(const char* multi_string) {
  const char* multi_string_end;
  return ExtractMultiStringElements(multi_string, &multi_string_end);
}

}  // namespace google_smart_card
