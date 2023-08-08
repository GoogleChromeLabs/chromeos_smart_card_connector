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

#ifndef GOOGLE_SMART_CARD_COMMON_VALUE_BUILDER_H_
#define GOOGLE_SMART_CARD_COMMON_VALUE_BUILDER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/cpp/src/public/value.h"
#include "common/cpp/src/public/value_conversion.h"

namespace google_smart_card {

// Helper for simplifying code that creates an array `Value`.
//
// Usage example:
//   Value x = ArrayValueBuilder().Add("x").Add(123).Get();
class ArrayValueBuilder final {
 public:
  ArrayValueBuilder();
  ArrayValueBuilder(const ArrayValueBuilder&) = delete;
  ArrayValueBuilder& operator=(const ArrayValueBuilder&) = delete;
  ~ArrayValueBuilder();

  // Adds a new item with the given value (converted into a `Value` if needed).
  template <typename T>
  ArrayValueBuilder&& Add(T item) && {
    std::string conversion_error_message;
    Value converted;
    bool conversion_success =
        ConvertToValue(std::move(item), &converted, &conversion_error_message);
    AddConverted(conversion_success, conversion_error_message,
                 std::move(converted));
    return std::move(*this);
  }

  bool encountered_error() const { return encountered_error_; }
  const std::string& error_message() const { return error_message_; }

  // Returns the built array value. Dies if a conversion failure has been
  // encountered.
  Value Get() &&;

 private:
  void AddConverted(bool conversion_success,
                    const std::string& conversion_error_message,
                    Value converted);

  bool encountered_error_ = false;
  std::string error_message_;
  std::vector<Value> items_;
};

// Helper for simplifying code that creates a dictionary `Value`.
//
// Usage example:
//   Value x = DictValueBuilder().Add("name", "x").Add("length", 123).Get();
class DictValueBuilder final {
 public:
  DictValueBuilder();
  DictValueBuilder(const DictValueBuilder&) = delete;
  DictValueBuilder& operator=(const DictValueBuilder&) = delete;
  ~DictValueBuilder();

  // Adds the given key and value (converted into a `Value` if needed).
  template <typename T>
  DictValueBuilder&& Add(const std::string& key, T value) && {
    std::string conversion_error_message;
    Value converted;
    bool conversion_success =
        ConvertToValue(std::move(value), &converted, &conversion_error_message);
    AddConverted(conversion_success, conversion_error_message, key,
                 std::move(converted));
    return std::move(*this);
  }

  bool encountered_error() const { return encountered_error_; }
  const std::string& error_message() const { return error_message_; }

  // Returns the built dictionary value. Dies if a conversion failure has been
  // encountered.
  Value Get() &&;

 private:
  void AddConverted(bool conversion_success,
                    const std::string& conversion_error_message,
                    const std::string& key,
                    Value converted_value);

  bool encountered_error_ = false;
  std::string error_message_;
  std::map<std::string, Value> items_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_BUILDER_H_
