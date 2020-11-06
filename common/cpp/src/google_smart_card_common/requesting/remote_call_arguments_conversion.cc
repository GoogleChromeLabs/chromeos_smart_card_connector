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

#include <google_smart_card_common/requesting/remote_call_arguments_conversion.h>

#include <string>
#include <utility>
#include <vector>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_debug_dumping.h>

namespace google_smart_card {

namespace internal {

// Definitions of the constants declared in the header file:
const char kRemoteCallArgumentConversionError[] =
    "Failed to convert argument #%d for %s(): %s";

}  // namespace internal

RemoteCallArgumentsExtractor::RemoteCallArgumentsExtractor(
    std::string title,
    std::vector<Value> argument_values)
    : title_(std::move(title)), argument_values_(std::move(argument_values)) {}

RemoteCallArgumentsExtractor::RemoteCallArgumentsExtractor(
    std::string title,
    Value arguments_as_value)
    : title_(std::move(title)) {
  if (!ConvertFromValue(std::move(arguments_as_value), &argument_values_,
                        &error_message_)) {
    success_ = false;
    error_message_ =
        FormatPrintfTemplate("Failed to convert arguments for %s(): %s",
                             title_.c_str(), error_message_.c_str());
  }
}

RemoteCallArgumentsExtractor::~RemoteCallArgumentsExtractor() = default;

void RemoteCallArgumentsExtractor::Extract() {}

bool RemoteCallArgumentsExtractor::Finish() {
  VerifyNothingLeft();
  return success_;
}

void RemoteCallArgumentsExtractor::VerifySufficientCount(
    int arguments_to_convert) {
  if (!success_)
    return;
  const size_t min_size = current_argument_index_ + arguments_to_convert;
  if (min_size <= argument_values_.size())
    return;
  success_ = false;
  error_message_ = FormatPrintfTemplate(
      "Failed to convert arguments for %s(): expected at least %d argument(s), "
      "received only %d",
      title_.c_str(), static_cast<int>(min_size),
      static_cast<int>(argument_values_.size()));
}

void RemoteCallArgumentsExtractor::VerifyNothingLeft() {
  if (!success_ || current_argument_index_ == argument_values_.size())
    return;
  success_ = false;
  error_message_ = FormatPrintfTemplate(
      "Failed to convert arguments for %s(): expected exactly %d arguments, "
      "received %d; first extra argument: %s",
      title_.c_str(), static_cast<int>(current_argument_index_),
      static_cast<int>(argument_values_.size()),
      DebugDumpValueSanitized(argument_values_[current_argument_index_])
          .c_str());
}

}  // namespace google_smart_card
