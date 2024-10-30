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

#include "common/cpp/src/public/value.h"

#include <string>
#include <utility>
#include <vector>

#include "common/cpp/src/public/logging/logging.h"

namespace google_smart_card {

const char* const Value::kNullTypeTitle = "null";
const char* const Value::kBooleanTypeTitle = "boolean";
const char* const Value::kIntegerTypeTitle = "integer";
const char* const Value::kFloatTypeTitle = "float";
const char* const Value::kStringTypeTitle = "string";
const char* const Value::kBinaryTypeTitle = "binary";
const char* const Value::kDictionaryTypeTitle = "dictionary";
const char* const Value::kArrayTypeTitle = "array";

namespace {

Value::DictionaryStorage ConvertMapToUniquePtrMap(
    std::map<std::string, Value> map) {
  Value::DictionaryStorage dictionary_storage;
  for (auto& item : map)
    dictionary_storage[item.first] =
        std::make_unique<Value>(std::move(item.second));
  return dictionary_storage;
}

Value::ArrayStorage ConvertValuesToUniquePtrs(std::vector<Value> values) {
  Value::ArrayStorage array_storage;
  array_storage.reserve(values.size());
  for (auto& value : values)
    array_storage.push_back(std::make_unique<Value>(std::move(value)));
  return array_storage;
}

}  // namespace

Value::Value() {
  // Note: Cannot use "= default", because the inner union's default constructor
  // is deleted.
}

Value::Value(Type type) : type_(type) {
  // Default-initialize the corresponding field.
  switch (type_) {
    case Type::kNull:
      break;
    case Type::kBoolean:
      boolean_value_ = false;
      break;
    case Type::kInteger:
      integer_value_ = 0;
      break;
    case Type::kFloat:
      float_value_ = 0;
      break;
    case Type::kString:
      new (&string_value_) std::string();
      break;
    case Type::kBinary:
      new (&binary_value_) BinaryStorage();
      break;
    case Type::kDictionary:
      new (&dictionary_value_) DictionaryStorage();
      break;
    case Type::kArray:
      new (&array_value_) ArrayStorage();
      break;
  }
}

Value::Value(bool boolean_value)
    : type_(Type::kBoolean), boolean_value_(boolean_value) {}

Value::Value(int integer_value)
    : type_(Type::kInteger), integer_value_(integer_value) {}

Value::Value(int64_t integer_value)
    : type_(Type::kInteger), integer_value_(integer_value) {}

Value::Value(double float_value)
    : type_(Type::kFloat), float_value_(float_value) {}

Value::Value(const char* string_value) : Value(std::string(string_value)) {}

Value::Value(std::string string_value)
    : type_(Type::kString), string_value_(std::move(string_value)) {}

Value::Value(BinaryStorage binary_value)
    : type_(Type::kBinary), binary_value_(std::move(binary_value)) {}

Value::Value(DictionaryStorage dictionary_value)
    : type_(Type::kDictionary),
      dictionary_value_(std::move(dictionary_value)) {}

Value::Value(std::map<std::string, Value> dictionary_value)
    : Value(ConvertMapToUniquePtrMap(std::move(dictionary_value))) {}

Value::Value(ArrayStorage array_value)
    : type_(Type::kArray), array_value_(std::move(array_value)) {}

Value::Value(std::vector<Value> array_value)
    : Value(ConvertValuesToUniquePtrs(std::move(array_value))) {}

Value::Value(Value&& other) {
  MoveConstructFrom(std::move(other));
}

Value& Value::operator=(Value&& other) {
  if (this != &other) {
    Destroy();
    MoveConstructFrom(std::move(other));
  }
  return *this;
}

Value::~Value() {
  Destroy();
}

bool Value::StrictlyEquals(const Value& other) const {
  if (type_ != other.type_)
    return false;
  switch (type_) {
    case Type::kNull:
      return true;
    case Type::kBoolean:
      return boolean_value_ == other.boolean_value_;
    case Type::kInteger:
      return integer_value_ == other.integer_value_;
    case Type::kFloat:
      return float_value_ == other.float_value_;
    case Type::kString:
      return string_value_ == other.string_value_;
    case Type::kBinary:
      return binary_value_ == other.binary_value_;
    case Type::kDictionary: {
      if (dictionary_value_.size() != other.dictionary_value_.size())
        return false;
      for (const auto& pair : dictionary_value_) {
        const auto iter = other.dictionary_value_.find(pair.first);
        if (iter == other.dictionary_value_.end() ||
            !pair.second->StrictlyEquals(*iter->second)) {
          return false;
        }
      }
      return true;
    }
    case Type::kArray: {
      if (array_value_.size() != other.array_value_.size())
        return false;
      for (size_t i = 0; i < array_value_.size(); ++i) {
        if (!array_value_[i]->StrictlyEquals(*other.array_value_[i]))
          return false;
      }
      return true;
    }
  }
}

bool Value::GetBoolean() const {
  GOOGLE_SMART_CARD_CHECK(is_boolean());
  return boolean_value_;
}

int64_t Value::GetInteger() const {
  GOOGLE_SMART_CARD_CHECK(is_integer());
  return integer_value_;
}

double Value::GetFloat() const {
  GOOGLE_SMART_CARD_CHECK(is_integer() || is_float());
  return is_float() ? float_value_ : integer_value_;
}

const std::string& Value::GetString() const {
  GOOGLE_SMART_CARD_CHECK(is_string());
  return string_value_;
}

const Value::BinaryStorage& Value::GetBinary() const {
  GOOGLE_SMART_CARD_CHECK(is_binary());
  return binary_value_;
}

Value::BinaryStorage& Value::GetBinary() {
  GOOGLE_SMART_CARD_CHECK(is_binary());
  return binary_value_;
}

const Value::DictionaryStorage& Value::GetDictionary() const {
  GOOGLE_SMART_CARD_CHECK(is_dictionary());
  return dictionary_value_;
}

Value::DictionaryStorage& Value::GetDictionary() {
  GOOGLE_SMART_CARD_CHECK(is_dictionary());
  return dictionary_value_;
}

const Value::ArrayStorage& Value::GetArray() const {
  GOOGLE_SMART_CARD_CHECK(is_array());
  return array_value_;
}

Value::ArrayStorage& Value::GetArray() {
  GOOGLE_SMART_CARD_CHECK(is_array());
  return array_value_;
}

const Value* Value::GetDictionaryItem(const std::string& key) const {
  GOOGLE_SMART_CARD_CHECK(is_dictionary());
  auto iter = dictionary_value_.find(key);
  if (iter == dictionary_value_.end())
    return nullptr;
  return iter->second.get();
}

void Value::SetDictionaryItem(std::string key, Value value) {
  GOOGLE_SMART_CARD_CHECK(is_dictionary());
  auto iter = dictionary_value_.find(key);
  if (iter != dictionary_value_.end()) {
    *iter->second = std::move(value);
    return;
  }
  dictionary_value_.insert(
      iter, std::make_pair(std::move(key),
                           std::make_unique<Value>(std::move(value))));
}

void Value::MoveConstructFrom(Value&& other) {
  type_ = other.type_;
  switch (type_) {
    case Type::kNull:
      break;
    case Type::kBoolean:
      boolean_value_ = other.boolean_value_;
      break;
    case Type::kInteger:
      integer_value_ = other.integer_value_;
      break;
    case Type::kFloat:
      float_value_ = other.float_value_;
      break;
    case Type::kString:
      new (&string_value_) std::string(std::move(other.string_value_));
      break;
    case Type::kBinary:
      new (&binary_value_) BinaryStorage(std::move(other.binary_value_));
      break;
    case Type::kDictionary:
      new (&dictionary_value_)
          DictionaryStorage(std::move(other.dictionary_value_));
      break;
    case Type::kArray:
      new (&array_value_) ArrayStorage(std::move(other.array_value_));
      break;
  }
}

void Value::Destroy() {
  switch (type_) {
    case Type::kNull:
    case Type::kBoolean:
    case Type::kInteger:
    case Type::kFloat:
      break;
    case Type::kString:
      string_value_.~basic_string();
      break;
    case Type::kBinary:
      binary_value_.~BinaryStorage();
      break;
    case Type::kDictionary:
      dictionary_value_.~DictionaryStorage();
      break;
    case Type::kArray:
      array_value_.~ArrayStorage();
      break;
  }
}

}  // namespace google_smart_card
