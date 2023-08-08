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

#ifndef GOOGLE_SMART_CARD_COMMON_VALUE_H_
#define GOOGLE_SMART_CARD_COMMON_VALUE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace google_smart_card {

// A variant data type that approximately corresponds to JSONifiable types.
//
// Is intended to be used in generic interfaces related to message exchanging
// with remote callers/receivers, for instance, for sending/receiving messages
// to/from JavaScript code.
//
// Inspired by Chromium's base::Value, Pepper's pp::Var classes and JavaScript
// type system.
class Value final {
 public:
  enum class Type {
    kNull,
    kBoolean,
    kInteger,
    kFloat,
    kString,
    kBinary,
    kDictionary,
    kArray,
  };

  using BinaryStorage = std::vector<uint8_t>;
  using DictionaryStorage = std::map<std::string, std::unique_ptr<Value>>;
  using ArrayStorage = std::vector<std::unique_ptr<Value>>;

  // String representation of the `Type` enum item. Intended to be used for
  // logging purposes.
  static const char* const kNullTypeTitle;
  static const char* const kBooleanTypeTitle;
  static const char* const kIntegerTypeTitle;
  static const char* const kFloatTypeTitle;
  static const char* const kStringTypeTitle;
  static const char* const kBinaryTypeTitle;
  static const char* const kDictionaryTypeTitle;
  static const char* const kArrayTypeTitle;

  Value();
  explicit Value(Type type);
  explicit Value(bool boolean_value);
  explicit Value(int integer_value);
  explicit Value(int64_t integer_value);
  explicit Value(double float_value);
  explicit Value(const char* string_value);
  explicit Value(std::string string_value);
  explicit Value(BinaryStorage binary_value);
  explicit Value(DictionaryStorage dictionary_value);
  explicit Value(std::map<std::string, Value> dictionary_value);
  explicit Value(ArrayStorage array_value);
  explicit Value(std::vector<Value> array_value);
  // Forbid construction from pointers other than `const char*`. Without this
  // deleted overload, the `bool`-argument version would be silently picked up.
  explicit Value(const void*) = delete;

  Value(Value&& other);
  Value& operator=(Value&& other);
  Value(const Value&) = delete;
  Value& operator=(const Value&) = delete;

  ~Value();

  // Returns whether the value has the exact same type and value. Note that
  // false is returned when comparing an integer and a float, even when their
  // numerical value is the same.
  bool StrictlyEquals(const Value& other) const;

  Type type() const { return type_; }
  bool is_null() const { return type_ == Type::kNull; }
  bool is_boolean() const { return type_ == Type::kBoolean; }
  bool is_integer() const { return type_ == Type::kInteger; }
  bool is_float() const { return type_ == Type::kFloat; }
  bool is_string() const { return type_ == Type::kString; }
  bool is_binary() const { return type_ == Type::kBinary; }
  bool is_dictionary() const { return type_ == Type::kDictionary; }
  bool is_array() const { return type_ == Type::kArray; }

  bool GetBoolean() const;
  int64_t GetInteger() const;
  double GetFloat() const;
  const std::string& GetString() const;
  const BinaryStorage& GetBinary() const;
  BinaryStorage& GetBinary();
  const DictionaryStorage& GetDictionary() const;
  DictionaryStorage& GetDictionary();
  const ArrayStorage& GetArray() const;
  ArrayStorage& GetArray();

  // Returns null when the key isn't present.
  const Value* GetDictionaryItem(const std::string& key) const;

  void SetDictionaryItem(std::string key, Value value);
  void SetDictionaryItem(std::string key, bool boolean_value) {
    SetDictionaryItem(std::move(key), Value(boolean_value));
  }
  void SetDictionaryItem(std::string key, int integer_value) {
    SetDictionaryItem(std::move(key), Value(integer_value));
  }
  void SetDictionaryItem(std::string key, int64_t integer_value) {
    SetDictionaryItem(std::move(key), Value(integer_value));
  }
  void SetDictionaryItem(std::string key, double float_value) {
    SetDictionaryItem(std::move(key), Value(float_value));
  }
  void SetDictionaryItem(std::string key, const char* string_value) {
    SetDictionaryItem(std::move(key), Value(string_value));
  }
  void SetDictionaryItem(std::string key, std::string string_value) {
    SetDictionaryItem(std::move(key), Value(std::move(string_value)));
  }
  void SetDictionaryItem(std::string key, BinaryStorage binary_value) {
    SetDictionaryItem(std::move(key), Value(std::move(binary_value)));
  }
  void SetDictionaryItem(std::string key, DictionaryStorage dictionary_value) {
    SetDictionaryItem(std::move(key), Value(std::move(dictionary_value)));
  }
  void SetDictionaryItem(std::string key, ArrayStorage array_value) {
    SetDictionaryItem(std::move(key), Value(std::move(array_value)));
  }

 private:
  void MoveConstructFrom(Value&& other);
  void Destroy();

  Type type_ = Type::kNull;
  union {
    bool boolean_value_;
    int64_t integer_value_;
    double float_value_;
    std::string string_value_;
    BinaryStorage binary_value_;
    DictionaryStorage dictionary_value_;
    ArrayStorage array_value_;
  };
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_H_
