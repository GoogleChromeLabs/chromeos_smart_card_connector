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

#ifndef GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_STRUCT_CONVERTER_H_
#define GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_STRUCT_CONVERTER_H_

#include <string>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/pp_var_utils/operations.h>

namespace google_smart_card {

// Helper class for performing conversions between C/C++ structures and Pepper
// values.
//
// The basic assumption for the conversion is that there is a one-to-one
// correspondence between C/C++ structure data members and the keys of the
// Pepper dictionary value.
//
// The only exception to this rule are the optional fields: the fields of the
// C/C++ structure that are instantiations of the google_smart_card::optional
// template can be absent from the Pepper dictionary value.
//
// In order to use this class with a specific StructType, the VisitFields and
// the GetStructTypeName methods have to be
// implemented for the corresponding StructConverter class specialization.
template <typename StructType>
class StructConverter final {
 public:
  // Converts C/C++ structure value into Pepper value.
  static pp::Var ConvertToVar(const StructType& value) {
    pp::VarDictionary result;
    VisitFields(value, ToVarConversionCallback(&result));
    return result;
  }

  // Converts Pepper value into C/C++ structure value.
  //
  // Fails if the Pepper value is not a dictionary, or if it contains some
  // unexpected extra keys, or if at least one key corresponding to a
  // non-optional field is missing.
  static bool ConvertFromVar(
      const pp::Var& var, StructType* result, std::string* error_message) {
    GOOGLE_SMART_CARD_CHECK(result);
    GOOGLE_SMART_CARD_CHECK(error_message);
    pp::VarDictionary var_dict;
    if (!VarAs(var, &var_dict, error_message))
      return false;
    FromVarConversionCallback callback(var_dict);
    VisitFields(*result, callback);
    return callback.GetSuccess(error_message);
  }

  // Returns the textual name of the StructType (used for displaying it in error
  // messages).
  //
  // The class consumer has to implement this method for the class
  // specialization that corresponds to this StructType.
  static constexpr const char* GetStructTypeName();

 private:
  // Generates a number of pairs: (C/C++ struct field pointer, field name) and
  // calls the specified callback with each of them.
  //
  // The class consumer has to implement this method for the class
  // specialization that corresponds to this StructType.
  //
  // The Callback will be the functor with the following signature:
  // void(FieldType, const std::string&),
  // where FieldType is an arbitrary template parameter.
  template <typename Callback>
  static void VisitFields(const StructType& value, Callback callback);

  // The functor that is passed to the VisitFields method when the conversion
  // from the C/C++ structure value into the Pepper value is performed.
  class ToVarConversionCallback final {
   public:
    explicit ToVarConversionCallback(pp::VarDictionary* target_var)
        : target_var_(target_var) {
      GOOGLE_SMART_CARD_CHECK(target_var_);
    }

    template <typename FieldType>
    void operator()(const FieldType* field, const std::string& field_name) {
      GOOGLE_SMART_CARD_CHECK(field);
      SetVarDictValue(target_var_, field_name, *field);
    }

    template <typename FieldType>
    void operator()(
        const optional<FieldType>* field, const std::string& field_name) {
      GOOGLE_SMART_CARD_CHECK(field);
      if (*field)
        SetVarDictValue(target_var_, field_name, **field);
    }

   private:
    pp::VarDictionary* target_var_;
  };

  // The functor that is passed to the VisitFields method when the conversion
  // from the Pepper value into the C/C++ structure value is performed.
  class FromVarConversionCallback final {
   public:
    explicit FromVarConversionCallback(const pp::VarDictionary& var)
        : extractor_(var) {}

    template <typename FieldType>
    void operator()(
        const FieldType* field,
        const std::string& field_name) {
      GOOGLE_SMART_CARD_CHECK(field);
      // Note: this const_cast is correct as this class is only used by
      // ConvertFromVar method, that operates with a mutable struct object.
      FieldType* const field_mutable = const_cast<FieldType*>(field);
      ProcessField(field_mutable, field_name);
    }

    bool GetSuccess(std::string* error_message) const {
      GOOGLE_SMART_CARD_CHECK(error_message);
      return extractor_.GetSuccess(error_message);
    }

   private:
    template <typename FieldType>
    void ProcessField(FieldType* field, const std::string& field_name) {
      extractor_.Extract(field_name, field);
    }

    template <typename FieldType>
    void ProcessField(
        optional<FieldType>* field, const std::string& field_name) {
      extractor_.TryExtractOptional(field_name, field);
    }

    VarDictValuesExtractor extractor_;
  };
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_STRUCT_CONVERTER_H_
