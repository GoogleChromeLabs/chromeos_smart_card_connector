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

#include <google_smart_card_common/formatting.h>

#include <cstdio>
#include <utility>
#include <vector>

namespace google_smart_card {

namespace {

constexpr size_t kInitialPrintfBufferSize = 100;

}  // namespace

std::string FormatPrintfTemplate(const char* format, ...) {
  va_list var_args;
  va_start(var_args, format);
  std::string result = FormatPrintfTemplate(format, var_args);
  va_end(var_args);
  return result;
}

std::string FormatPrintfTemplate(const char* format, va_list var_args) {
  std::vector<char> buffer(kInitialPrintfBufferSize);
  for (;;) {
    va_list var_args_copy;
    va_copy(var_args_copy, var_args);
    const int result =
        std::vsnprintf(&buffer[0], buffer.size(), format, var_args_copy);
    va_end(var_args_copy);
    if (0 <= result && result < static_cast<int>(buffer.size()))
      return std::string(buffer.begin(), buffer.begin() + result);
    const size_t new_size = result > 0 ? result + 1 : buffer.size() * 2;
    buffer.resize(new_size);
  }
}

void FormatPrintfTemplateAndSet(std::string* output_string,
                                const char* format,
                                ...) {
  if (!output_string)
    return;
  va_list var_args;
  va_start(var_args, format);
  *output_string = FormatPrintfTemplate(format, var_args);
  va_end(var_args);
}

}  // namespace google_smart_card
