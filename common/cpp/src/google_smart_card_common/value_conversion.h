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

// Helpers for converting between `Value` instances and usual non-variant C++
// types:
// * "bool ConvertToValue(T object, Value* value,
//                        std::string* error_message = nullptr)";
// * "bool ConvertFromValue(Value value, T* object,
//                          std::string* error_message = nullptr)".
//
// These helpers are implemented for many standard types:
// * `bool`;
// * `int`, `int64_t` and other similar integer types (note: there's also a
//   special case that an integer can be converted from a floating-point
//   `Value`, in case it's within the range of precisely representable numbers);
// * `double`;
// * `std::string`.
//
// The same helpers can also be enabled for custom types:
// * a custom enum can be registered via the `EnumValueDescriptor` class for
//   conversion to/from a string `Value`.
//
// TODO: Add support for std::vector, struct.

#ifndef GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_
#define GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_

#include <stdint.h>

#include <string>
#include <type_traits>
#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

///////////// Internal helpers ///////////////

namespace internal {

// Visitor of enum type's items that converts a C++ enum value into a string
// `Value`, by finding the corresponding item among visited ones. This is a
// non-template class, because enum values are casted to integers.
class EnumToValueConverter final {
 public:
  explicit EnumToValueConverter(int64_t enum_to_convert);
  EnumToValueConverter(const EnumToValueConverter&) = delete;
  EnumToValueConverter& operator=(const EnumToValueConverter&) = delete;
  ~EnumToValueConverter();

  void HandleItem(int64_t enum_item, const char* enum_item_name);
  bool TakeConvertedValue(const char* type_name, Value* converted_value,
                          std::string* error_message = nullptr);

 private:
  const int64_t enum_to_convert_;
  optional<Value> converted_value_;
};

// Visitor of enum type's items that converts a string `Value` into a C++ enum
// value, by finding the corresponding item among visited ones. This is a
// non-template class, because enum values are casted to integers.
class EnumFromValueConverter final {
 public:
  explicit EnumFromValueConverter(Value value_to_convert);
  EnumFromValueConverter(const EnumFromValueConverter&) = delete;
  EnumFromValueConverter& operator=(const EnumFromValueConverter&) = delete;
  ~EnumFromValueConverter();

  void HandleItem(int64_t enum_item, const char* enum_item_name);
  bool GetConvertedEnum(const char* type_name, int64_t* converted_enum,
                        std::string* error_message) const;

 private:
  const Value value_to_convert_;
  optional<int64_t> converted_enum_;
};

}  // namespace internal

///////////// EnumValueDescriptor ////////////

// Class that allows to describe an enum's items and their corresponding string
// names. Can be used in order to automatically implement conversion of a C++
// enum to/from a string `Value` object (provided by the global template
// functions `ConvertToValue()` and `ConvertFromValue()` defined below).
//
// Example usage:
//   enum class Tp { kA, kB };
//   template <> EnumValueDescriptor<Tp>::Description
//   EnumValueDescriptor<Tp>::GetDescription() {
//     return Describe("Tp").WithItem(Tp::kA, "a").WithItem(Tp::kB, "b");
//   }
//   ...
//   ConvertToValue(Tp::kA, ...);
//   ConvertFromValue(Value("b"), ...);
template <typename T>
class EnumValueDescriptor {
 public:
  static_assert(std::is_enum<T>::value,
                "The EnumValueDescriptor parameter must be an enum");

  // Class that should be used for describing items of the enum type. Should be
  // instantiated via the `Describe()` method.
  class Description final {
   public:
    Description(const char* type_name,
                internal::EnumToValueConverter* to_value_converter,
                internal::EnumFromValueConverter* from_value_converter)
        : type_name_(type_name),
          to_value_converter_(to_value_converter),
          from_value_converter_(from_value_converter) {}
    Description(Description&&) = default;
    Description& operator=(Description&&) = default;
    ~Description() = default;

    const char* type_name() const { return type_name_; }

    // Adds the given item into the enum's description: |enum_item_name| is the
    // `Value` representation of |enum_item|.
    // Returns an rvalue reference to |this| and uses the "&&" ref-qualifier, so
    // that the method calls can be easily chained and the final result can be
    // returned without an explicit std::move() boilerplate.
    Description&& WithItem(T enum_item, const char* enum_item_name) && {
      const int64_t enum_item_number = static_cast<int64_t>(enum_item);
      if (to_value_converter_)
        to_value_converter_->HandleItem(enum_item_number, enum_item_name);
      if (from_value_converter_)
        from_value_converter_->HandleItem(enum_item_number, enum_item_name);
      return std::move(*this);
    }

   private:
    const char* const type_name_;
    internal::EnumToValueConverter* const to_value_converter_;
    internal::EnumFromValueConverter* const from_value_converter_;
  };

  EnumValueDescriptor(internal::EnumToValueConverter* to_value_converter,
                      internal::EnumFromValueConverter* from_value_converter)
      : to_value_converter_(to_value_converter),
        from_value_converter_(from_value_converter) {}
  EnumValueDescriptor(const EnumValueDescriptor&) = delete;
  EnumValueDescriptor& operator=(const EnumValueDescriptor&) = delete;
  ~EnumValueDescriptor() = default;

  // Not implemented in the template class; its implementation has to be
  // provided for every `T` by the consumer of this class.
  Description GetDescription();

 private:
  // Creates the `Description` object; intended to be used by the
  // `GetDescription()` implementation in order to describe all items of the
  // enum type via this object.
  Description Describe(const char* type_name) {
    return Description(type_name, to_value_converter_, from_value_converter_);
  }

  internal::EnumToValueConverter* const to_value_converter_;
  internal::EnumFromValueConverter* const from_value_converter_;
};

///////////// ConvertToValue /////////////////

// Group of overloads that perform trivial conversions to `Value`.
//
// Note: There are intentionally no specializations for constructing from
// `Value::BinaryStorage`, `Value::ArrayStorage`, `Value::DictionaryStorage`,
// since the first two are handled via a generic `std::vector` overload below,
// and the last one isn't useful in this context (as helpers in this file are
// about converting between a `Value` and a non-`Value` object).

inline bool ConvertToValue(bool boolean, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(boolean);
  return true;
}

inline bool ConvertToValue(int number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

bool ConvertToValue(unsigned number, Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(long number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(static_cast<int64_t>(number));
  return true;
}

bool ConvertToValue(unsigned long number, Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(uint8_t number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

inline bool ConvertToValue(int64_t number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

inline bool ConvertToValue(double number, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

// Note: `characters` must be non-null.
bool ConvertToValue(const char* characters, Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(std::string characters, Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(std::move(characters));
  return true;
}

// Forbid conversion of pointers other than `const char*`. Without this deleted
// overload, the `bool`-argument overload would be silently picked up.
bool ConvertToValue(const void* pointer_value, Value* value,
                    std::string* error_message = nullptr) = delete;

// Converts from an enum into a string `Value`. The enum type has to be
// registered via the `EnumValueDescriptor` class.
// Note: The |typename enable_if<...>::type| part boils down to |bool| for all
// |T| that are enums; for all other |T| this removes this template from
// overload resolution, so that non-enum types will use other functions declared
// in this file.
template <typename T>
typename std::enable_if<std::is_enum<T>::value, bool>::type ConvertToValue(
    T enum_value, Value* value, std::string* error_message = nullptr) {
  internal::EnumToValueConverter converter(static_cast<int64_t>(enum_value));
  const auto& description =
      EnumValueDescriptor<T>(&converter, /*from_value_converter=*/nullptr)
          .GetDescription();
  return converter.TakeConvertedValue(description.type_name(), value,
                                      error_message);
}

///////////// ConvertFromValue ///////////////

// Group of overloads that perform trivial conversions from `Value`.
bool ConvertFromValue(Value value, bool* boolean,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, int* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, unsigned* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, long* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, unsigned long* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, uint8_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, int64_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, double* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value, std::string* characters,
                      std::string* error_message = nullptr);

// Converts from a string `Value` into an enum. The enum type has to be
// registered via the `EnumValueDescriptor` class.
// Note: The |typename enable_if<...>::type| part boils down to |bool| for all
// |T| that are enums; for all other |T| this removes this template from
// overload resolution, so that non-enum types will use other functions declared
// in this file.
template <typename T>
typename std::enable_if<std::is_enum<T>::value, bool>::type ConvertFromValue(
    Value value, T* enum_value, std::string* error_message = nullptr) {
  internal::EnumFromValueConverter converter(std::move(value));
  const auto& description =
      EnumValueDescriptor<T>(/*from_value_converter=*/nullptr, &converter)
          .GetDescription();
  int64_t converted_enum_number;
  if (!converter.GetConvertedEnum(description.type_name(),
                                  &converted_enum_number, error_message)) {
    return false;
  }
  *enum_value = static_cast<T>(converted_enum_number);
  return true;
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_
