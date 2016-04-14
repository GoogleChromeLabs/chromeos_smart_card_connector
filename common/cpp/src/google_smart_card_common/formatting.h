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

// Helper functions providing printf-style string formatting.

#ifndef GOOGLE_SMART_CARD_COMMON_FORMATTING_H_
#define GOOGLE_SMART_CARD_COMMON_FORMATTING_H_

#include <cstdarg>
#include <string>

#include <boost/format.hpp>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

std::string FormatPrintfTemplate(const char* format, ...);

std::string FormatPrintfTemplate(const char* format, va_list var_args);

namespace internal {

inline void ApplyBoostFormatArguments(
    const std::string& format, boost::format* formatter) {
  if (formatter->remaining_args()) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Failed to format boost format template " <<
        "\"" << format << "\": supplied fewer arguments than it was expected";
  }
}

template <typename Arg, typename ... Args>
inline void ApplyBoostFormatArguments(
    const std::string& format,
    boost::format* formatter,
    const Arg& arg,
    const Args& ... args) {
  if (!formatter->remaining_args()) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Failed to format boost format template " <<
        "\"" << format << "\": supplied more arguments than it was expected";
  }
  *formatter % arg;
  ApplyBoostFormatArguments(format, formatter, args...);
}

}  // namespace internal

template <typename ... Args>
inline std::string FormatBoostFormatTemplate(
    const std::string& format, const Args& ... args) {
  boost::format formatter(format);
  internal::ApplyBoostFormatArguments(format, &formatter, args...);
  return formatter.str();
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_FORMATTING_H_
