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

#include "common/cpp/src/public/value_builder.h"

#include <string>
#include <utility>

#include "common/cpp/src/public/formatting.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/value.h"

namespace google_smart_card {

ArrayValueBuilder::ArrayValueBuilder() = default;

ArrayValueBuilder::~ArrayValueBuilder() = default;

void ArrayValueBuilder::AddConverted(
    bool conversion_success,
    const std::string& conversion_error_message,
    Value converted) {
  if (encountered_error_)
    return;
  if (!conversion_success) {
    encountered_error_ = true;
    error_message_ = FormatPrintfTemplate("Failed to convert item#%d: %s",
                                          static_cast<int>(items_.size()),
                                          conversion_error_message.c_str());
    return;
  }
  items_.push_back(std::move(converted));
}

Value ArrayValueBuilder::Get() && {
  if (encountered_error_)
    GOOGLE_SMART_CARD_LOG_FATAL << "Array building failed: " << error_message_;
  return Value(std::move(items_));
}

DictValueBuilder::DictValueBuilder() = default;

DictValueBuilder::~DictValueBuilder() = default;

void DictValueBuilder::AddConverted(bool conversion_success,
                                    const std::string& conversion_error_message,
                                    const std::string& key,
                                    Value converted_value) {
  if (encountered_error_)
    return;
  if (!conversion_success) {
    encountered_error_ = true;
    error_message_ =
        FormatPrintfTemplate(R"(Failed to convert key "%s": %s)", key.c_str(),
                             conversion_error_message.c_str());
    return;
  }
  if (items_.count(key)) {
    encountered_error_ = true;
    error_message_ = FormatPrintfTemplate(R"(Duplicate key "%s")", key.c_str());
    return;
  }
  items_[key] = std::move(converted_value);
}

Value DictValueBuilder::Get() && {
  if (encountered_error_) {
    GOOGLE_SMART_CARD_LOG_FATAL << "Dictionary building failed: "
                                << error_message_;
  }
  return Value(std::move(items_));
}

}  // namespace google_smart_card
