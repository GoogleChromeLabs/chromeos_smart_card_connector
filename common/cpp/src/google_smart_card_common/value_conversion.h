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
// * `std::string`;
// * `std::nullptr_t` (converts to/from a null `Value`);
// * `std::vector` of any supported type (note: there's also a special case that
//    `std::vector<uint8_t>` is converted to/from a binary `Value` and can
//    additionally be converted from an array `Value`).
//
// The same helpers can also be enabled for custom types:
// * a custom enum can be registered via the `EnumValueDescriptor` class for
//   conversion to/from a string `Value`;
// * a custom struct can be registered via the `StructValueDescriptor` class for
//   conversion to/from a dictionary `Value`.

#ifndef GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_
#define GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_

#include <stdint.h>

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/unique_ptr_utils.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_debug_dumping.h>

namespace google_smart_card {

///////////// Internal helpers ///////////////////

namespace internal {

extern const char kErrorWrongTypeValueConversion[];
extern const char kErrorFromArrayValueConversion[];
extern const char kErrorToArrayValueConversion[];

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
  bool TakeConvertedValue(const char* type_name,
                          Value* converted_value,
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
  bool GetConvertedEnum(const char* type_name,
                        int64_t* converted_enum,
                        std::string* error_message) const;

 private:
  const Value value_to_convert_;
  optional<int64_t> converted_enum_;
};

// Base class for all `StructToValueConverter`s. Contains code that doesn't
// depend on the type of the struct and can therefore be shared between all
// child classes.
class StructToValueConverterBase {
 protected:
  StructToValueConverterBase();
  StructToValueConverterBase(const StructToValueConverterBase&) = delete;
  StructToValueConverterBase& operator=(const StructToValueConverterBase&) =
      delete;
  ~StructToValueConverterBase();

  void HandleFieldConversionError(const char* dictionary_key_name);
  bool FinishConversion(const char* type_name,
                        std::string* error_message) const;

  bool error_encountered_ = false;
  std::string inner_error_message_;
  Value converted_value_{Value::Type::kDictionary};
};

// Visitor of struct type's fields that converts a C++ struct into a dictionary
// `Value`.
template <typename T>
class StructToValueConverter final : public StructToValueConverterBase {
 public:
  explicit StructToValueConverter(T object_to_convert)
      : object_to_convert_(std::move(object_to_convert)) {}
  StructToValueConverter(const StructToValueConverter&) = delete;
  StructToValueConverter& operator=(const StructToValueConverter&) = delete;
  ~StructToValueConverter() = default;

  template <typename FieldT>
  void HandleField(FieldT T::*field_ptr, const char* dictionary_key_name) {
    FieldT& field = object_to_convert_.*field_ptr;
    ConvertFieldToValue(std::move(field), dictionary_key_name);
  }

  template <typename FieldT>
  void HandleField(optional<FieldT> T::*field_ptr,
                   const char* dictionary_key_name) {
    optional<FieldT>& field = object_to_convert_.*field_ptr;
    if (!field) {
      // The optional field is null - skip it from the conversion.
      return;
    }
    ConvertFieldToValue(std::move(*field), dictionary_key_name);
  }

  bool TakeConvertedValue(const char* type_name,
                          Value* converted_value,
                          std::string* error_message = nullptr) {
    if (!FinishConversion(type_name, error_message))
      return false;
    *converted_value = std::move(converted_value_);
    return true;
  }

 private:
  template <typename FieldT>
  void ConvertFieldToValue(FieldT field_value, const char* dictionary_key_name);

  T object_to_convert_;
};

// Base class for all `StructFromValueConverter`s. Contains code that doesn't
// depend on the type of the struct and can therefore be shared between all
// child classes.
class StructFromValueConverterBase {
 protected:
  explicit StructFromValueConverterBase(Value value_to_convert);
  StructFromValueConverterBase(const StructFromValueConverterBase&) = delete;
  StructFromValueConverterBase& operator=(const StructFromValueConverterBase&) =
      delete;
  ~StructFromValueConverterBase();

  bool ExtractKey(const char* dictionary_key_name,
                  bool is_required,
                  Value* item_value);
  void HandleFieldConversionError(const char* dictionary_key_name);
  bool FinishConversion(const char* type_name,
                        std::string* error_message = nullptr);

  Value value_to_convert_;
  bool error_encountered_ = false;
  bool permit_unexpected_keys_ = false;
  std::string inner_error_message_;
};

// Visitor of struct type's fields that converts a dictionary `Value` into a C++
// struct into.
template <typename T>
class StructFromValueConverter final : public StructFromValueConverterBase {
 public:
  explicit StructFromValueConverter(Value value_to_convert)
      : StructFromValueConverterBase(std::move(value_to_convert)) {}
  StructFromValueConverter(const StructFromValueConverter&) = delete;
  StructFromValueConverter& operator=(const StructFromValueConverter&) = delete;
  ~StructFromValueConverter() = default;

  template <typename FieldT>
  void HandleField(FieldT T::*field_ptr, const char* dictionary_key_name) {
    Value item_value;
    if (!ExtractKey(dictionary_key_name, /*is_required=*/true, &item_value))
      return;
    FieldT& field = converted_object_.*field_ptr;
    ConvertFieldFromValue(dictionary_key_name, std::move(item_value), &field);
  }

  template <typename FieldT>
  void HandleField(optional<FieldT> T::*field_ptr,
                   const char* dictionary_key_name) {
    Value item_value;
    if (!ExtractKey(dictionary_key_name, /*is_required=*/false, &item_value))
      return;
    optional<FieldT>& field = converted_object_.*field_ptr;
    field = FieldT();
    ConvertFieldFromValue(dictionary_key_name, std::move(item_value),
                          &field.value());
  }

  bool TakeConvertedObject(const char* type_name,
                           T* converted_object,
                           std::string* error_message = nullptr) {
    if (!FinishConversion(type_name, error_message))
      return false;
    *converted_object = std::move(converted_object_);
    return true;
  }

  void PermitUnexpectedKeys() { permit_unexpected_keys_ = true; }

 private:
  template <typename FieldT>
  void ConvertFieldFromValue(const char* dictionary_key_name,
                             Value item_value,
                             FieldT* converted_field);

  T converted_object_;
};

}  // namespace internal

///////////// EnumValueDescriptor ////////////////

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

///////////// StructValueDescriptor //////////////

// Class that allows to describe a struct's fields and their corresponding
// string names. Can be used in order to automatically implement conversion of a
// C++ struct to/from a dictionary `Value` object (provided by the global
// template functions `ConvertToValue()` and `ConvertFromValue()` defined
// below).
//
// Example usage:
//   struct Foo { int x; std::string y; };
//   template <> StructValueDescriptor<Tp>::Description
//   StructValueDescriptor<Tp>::GetDescription() {
//     return Describe("Foo").WithField(&Foo::x, "x").WithField(&Foo::y, "y");
//   }
//   ...
//   ConvertToValue(Foo{123, ""}, ...);
//   ConvertFromValue(Value(...), &foo, ...);
template <typename T>
class StructValueDescriptor {
 public:
  static_assert(std::is_class<T>::value,
                "The StructValueDescriptor parameter must be a class");

  // Class that should be used for describing fields of the struct type. Should
  // be instantiated via the `Describe()` method.
  class Description final {
   public:
    Description(const char* type_name,
                internal::StructToValueConverter<T>* to_value_converter,
                internal::StructFromValueConverter<T>* from_value_converter)
        : type_name_(type_name),
          to_value_converter_(to_value_converter),
          from_value_converter_(from_value_converter) {}
    Description(Description&&) = default;
    Description& operator=(Description&&) = default;
    ~Description() = default;

    const char* type_name() const { return type_name_; }

    // Adds the given field into the struct's description: |dictionary_key_name|
    // is the key in the dictionary `Value` representation of |field_ptr|.
    // Returns an rvalue reference to |this| and uses the "&&" ref-qualifier, so
    // that the method calls can be easily chained and the final result can be
    // returned without an explicit std::move() boilerplate.
    template <typename FieldT>
    Description&& WithField(FieldT T::*field_ptr,
                            const char* dictionary_key_name) && {
      if (to_value_converter_)
        to_value_converter_->HandleField(field_ptr, dictionary_key_name);
      else
        from_value_converter_->HandleField(field_ptr, dictionary_key_name);
      return std::move(*this);
    }

    Description&& PermitUnknownFields() && {
      if (from_value_converter_)
        from_value_converter_->PermitUnexpectedKeys();
      return std::move(*this);
    }

   private:
    const char* const type_name_;
    internal::StructToValueConverter<T>* const to_value_converter_;
    internal::StructFromValueConverter<T>* const from_value_converter_;
  };

  StructValueDescriptor(
      internal::StructToValueConverter<T>* to_value_converter,
      internal::StructFromValueConverter<T>* from_value_converter)
      : to_value_converter_(to_value_converter),
        from_value_converter_(from_value_converter) {}
  StructValueDescriptor(const StructValueDescriptor&) = delete;
  StructValueDescriptor& operator=(const StructValueDescriptor&) = delete;
  ~StructValueDescriptor() = default;

  // Not implemented in the template class; its implementation has to be
  // provided for every `T` by the consumer of this class.
  Description GetDescription();

 private:
  // Creates the `Description` object; intended to be used by the
  // `GetDescription()` implementation in order to describe all fields of the
  // struct type via this object.
  Description Describe(const char* type_name) {
    return Description(type_name, to_value_converter_, from_value_converter_);
  }

  internal::StructToValueConverter<T>* const to_value_converter_;
  internal::StructFromValueConverter<T>* const from_value_converter_;
};

// Explicitly forbid instantiating this class with unwanted types. Without doing
// this, the errors will appear only at the linking stage and without mentioning
// the place in the code that misuses this class.
template <typename T>
class StructValueDescriptor<optional<T>>;

///////////// ConvertToValue /////////////////////

// Group of overloads that perform trivial conversions to `Value`.
//
// Note: There are intentionally no specializations for constructing from
// `Value::BinaryStorage`, `Value::ArrayStorage`, `Value::DictionaryStorage`,
// since the first two are handled via a generic `std::vector` overload below,
// and the last one isn't useful in this context (as helpers in this file are
// about converting between a `Value` and a non-`Value` object).

inline bool ConvertToValue(Value::Type value_type,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(value_type);
  return true;
}

inline bool ConvertToValue(Value source_value,
                           Value* target_value,
                           std::string* /*error_message*/ = nullptr) {
  *target_value = std::move(source_value);
  return true;
}

inline bool ConvertToValue(bool boolean,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(boolean);
  return true;
}

inline bool ConvertToValue(int number,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

bool ConvertToValue(unsigned number,
                    Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(long number,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(static_cast<int64_t>(number));
  return true;
}

bool ConvertToValue(unsigned long number,
                    Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(uint8_t number,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

inline bool ConvertToValue(int64_t number,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

inline bool ConvertToValue(double number,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(number);
  return true;
}

// Note: `characters` must be non-null.
bool ConvertToValue(const char* characters,
                    Value* value,
                    std::string* error_message = nullptr);

inline bool ConvertToValue(std::string characters,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value(std::move(characters));
  return true;
}

// Converts `std::nullptr_t` into a null `Value`.
inline bool ConvertToValue(std::nullptr_t /*null*/,
                           Value* value,
                           std::string* /*error_message*/ = nullptr) {
  *value = Value();
  return true;
}

// Forbid conversion of pointers other than `const char*`. Without this deleted
// overload, the `bool`-argument overload would be silently picked up.
bool ConvertToValue(const void* pointer_value,
                    Value* value,
                    std::string* error_message = nullptr) = delete;

// Converts from an enum into a string `Value`. The enum type has to be
// registered via the `EnumValueDescriptor` class.
// Note: The |typename enable_if<...>::type| part boils down to |bool| for all
// |T| that are enums; for all other |T| this removes this template from
// overload resolution, so that non-enum types will use other functions declared
// in this file.
template <typename T>
typename std::enable_if<std::is_enum<T>::value, bool>::type ConvertToValue(
    T enum_value,
    Value* value,
    std::string* error_message = nullptr) {
  internal::EnumToValueConverter converter(static_cast<int64_t>(enum_value));
  const auto& description =
      EnumValueDescriptor<T>(&converter, /*from_value_converter=*/nullptr)
          .GetDescription();
  return converter.TakeConvertedValue(description.type_name(), value,
                                      error_message);
}

// Converts from a struct into a dictionary `Value`. The struct type has to be
// registered via the `StructValueDescriptor` class.
// Note: The |typename enable_if<...>::type| part boils down to |bool| for all
// |T| that are class/struct; for all other |T| this removes this template from
// overload resolution, so that unrelated types will use other functions
// declared in this file.
template <typename T>
typename std::enable_if<std::is_class<T>::value, bool>::type
ConvertToValue(T object, Value* value, std::string* error_message = nullptr) {
  internal::StructToValueConverter<T> converter(std::move(object));
  const auto& description =
      StructValueDescriptor<T>(&converter, /*from_value_converter=*/nullptr)
          .GetDescription();
  return converter.TakeConvertedValue(description.type_name(), value,
                                      error_message);
}

// Converts from a vector of items of a supported type into an array `Value`.
template <typename T>
bool ConvertToValue(std::vector<T> objects,
                    Value* value,
                    std::string* error_message = nullptr) {
  std::vector<Value> converted_items(objects.size());
  std::string local_error_message;
  for (size_t i = 0; i < objects.size(); ++i) {
    if (!ConvertToValue(std::move(objects[i]), &converted_items[i],
                        &local_error_message)) {
      FormatPrintfTemplateAndSet(
          error_message, internal::kErrorToArrayValueConversion,
          static_cast<int>(i), local_error_message.c_str());
      return false;
    }
  }
  *value = Value(std::move(converted_items));
  return true;
}

// Converts a vector of bytes into a binary `Value`. (Note: This is unlike all
// other types of `std::vector`, which are converted to an array `Value`.)
bool ConvertToValue(std::vector<uint8_t> bytes,
                    Value* value,
                    std::string* error_message = nullptr);

// Synonym to other `ConvertToValue()` overloads, but immediately crashes the
// program if the conversion fails.
template <typename T>
Value ConvertToValueOrDie(T object) {
  Value value;
  std::string error_message;
  if (!ConvertToValue(std::move(object), &value, &error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  return value;
}

///////////// ConvertFromValue ///////////////////

// Group of overloads that perform trivial conversions from `Value`.

inline bool ConvertFromValue(Value source_value,
                             Value* target_value,
                             std::string* /*error_message*/ = nullptr) {
  *target_value = std::move(source_value);
  return true;
}

bool ConvertFromValue(Value value,
                      bool* boolean,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      int* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      unsigned* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      long* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      unsigned long* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      uint8_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      int16_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      uint16_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      int64_t* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      double* number,
                      std::string* error_message = nullptr);
bool ConvertFromValue(Value value,
                      std::string* characters,
                      std::string* error_message = nullptr);

// Verifies that the `value` is null.
bool ConvertFromValue(Value value,
                      std::nullptr_t* null,
                      std::string* error_message = nullptr);

// Converts from a string `Value` into an enum. The enum type has to be
// registered via the `EnumValueDescriptor` class.
// Note: The |typename enable_if<...>::type| part boils down to |bool| for all
// |T| that are enums; for all other |T| this removes this template from
// overload resolution, so that non-enum types will use other functions declared
// in this file.
template <typename T>
typename std::enable_if<std::is_enum<T>::value, bool>::type ConvertFromValue(
    Value value,
    T* enum_value,
    std::string* error_message = nullptr) {
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

// Converts from a dictionary `Value` into a struct. The struct type has to be
// registered via the `StructValueDescriptor` class.
// Note: The |typename enable_if<...>::type| part boils down to |bool| for all
// |T| that are class/struct; for all other |T| this removes this template from
// overload resolution, so that unrelated types will use other functions
// declared in this file.
template <typename T>
typename std::enable_if<std::is_class<T>::value, bool>::type
ConvertFromValue(Value value, T* object, std::string* error_message = nullptr) {
  internal::StructFromValueConverter<T> converter(std::move(value));
  const auto& description =
      StructValueDescriptor<T>(/*to_value_converter=*/nullptr, &converter)
          .GetDescription();
  return converter.TakeConvertedObject(description.type_name(), object,
                                       error_message);
}

// Converts from an array `Value` into a vector of items of a supported type.
template <typename T>
bool ConvertFromValue(Value value,
                      std::vector<T>* objects,
                      std::string* error_message = nullptr) {
  if (!value.is_array()) {
    FormatPrintfTemplateAndSet(
        error_message, internal::kErrorWrongTypeValueConversion,
        Value::kArrayTypeTitle, DebugDumpValueSanitized(value).c_str());
    return false;
  }
  Value::ArrayStorage& array = value.GetArray();
  objects->resize(array.size());
  std::string local_error_message;
  for (size_t i = 0; i < array.size(); ++i) {
    if (!ConvertFromValue(std::move(*array[i]), &(*objects)[i],
                          &local_error_message)) {
      FormatPrintfTemplateAndSet(
          error_message, internal::kErrorFromArrayValueConversion,
          static_cast<int>(i), local_error_message.c_str());
      return false;
    }
  }
  return true;
}

// Converts from an array or binary `Value` into a vector of bytes. (Note: The
// difference to the template version above is the support of binary `Value`.)
bool ConvertFromValue(Value value,
                      std::vector<uint8_t>* bytes,
                      std::string* error_message = nullptr);

// Synonym to other `ConvertFromValue()` overloads, but immediately crashes the
// program if the conversion fails.
template <typename T>
T ConvertFromValueOrDie(Value value) {
  T object;
  std::string error_message;
  if (!ConvertFromValue(std::move(value), &object, &error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  return object;
}

///////////// Internal helpers implementation ////

// Note: These helpers are implemented here at the end of the file, so that they
// can call into other helpers declared above.

namespace internal {

template <typename T>
template <typename FieldT>
void StructToValueConverter<T>::ConvertFieldToValue(
    FieldT field_value,
    const char* dictionary_key_name) {
  if (error_encountered_)
    return;
  Value converted_field;
  if (ConvertToValue(std::move(field_value), &converted_field,
                     &inner_error_message_)) {
    converted_value_.SetDictionaryItem(dictionary_key_name,
                                       std::move(converted_field));
  } else {
    HandleFieldConversionError(dictionary_key_name);
  }
}

template <typename T>
template <typename FieldT>
void StructFromValueConverter<T>::ConvertFieldFromValue(
    const char* dictionary_key_name,
    Value item_value,
    FieldT* converted_field) {
  if (!ConvertFromValue(std::move(item_value), converted_field,
                        &inner_error_message_))
    HandleFieldConversionError(dictionary_key_name);
}

}  // namespace internal

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_VALUE_CONVERSION_H_
