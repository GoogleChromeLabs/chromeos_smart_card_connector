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

#ifndef GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_ENUM_CONVERTER_H_
#define GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_ENUM_CONVERTER_H_

#include <stdint.h>

#include <functional>
#include <string>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

namespace google_smart_card {

// Converts between C/C++ enum value and Pepper values.
//
// The basic assumption for the conversion is that there is a one-to-one
// correspondence between the C/C++ enum values and strings stored in the Pepper
// values.
//
// The EnumType should be the C/C++ enum type, and VarValueType should be the
// type of the Pepper values (it is assumed that all enum values of this
// EnumType are converted into the Pepper values of the same type).
//
// In order to use this class with a specific EnumType, the
// GenerateCorrespondingPairs and the GetEnumTypeName methods have to be
// implemented for the corresponding EnumConverter class specialization.
template <typename EnumType, typename VarValueType>
class EnumConverter final {
 public:
  EnumConverter() = default;
  EnumConverter(const EnumConverter&) = delete;

  // Converts C/C++ enum value into Pepper value.
  //
  // Crashes if the enum value is unknown.
  static pp::Var ConvertToVar(EnumType enum_value) {
    bool succeeded = false;
    pp::Var result;
    VisitCorrespondingPairs([&enum_value, &succeeded, &result](
        EnumType candidate_value, const VarValueType& candidate_var_value) {
      if (enum_value == candidate_value) {
        GOOGLE_SMART_CARD_CHECK(!succeeded);
        succeeded = true;
        result = candidate_var_value;
      }
    });
    if (!succeeded) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Failed to convert " << GetEnumTypeName()
          << " enum value " << static_cast<int64_t>(enum_value);
    }
    return result;
  }

  // Converts Pepper value into C/C++ enum value.
  //
  // Fails if the Pepper value has unexpected type, or if the value is not
  // corresponding to any C/C++ enum value.
  static bool ConvertFromVar(
      const pp::Var& var, EnumType* result, std::string* error_message) {
    GOOGLE_SMART_CARD_CHECK(result);
    GOOGLE_SMART_CARD_CHECK(error_message);

    VarValueType var_value;
    if (!VarAs(var, &var_value, error_message)) {
      *error_message = FormatBoostFormatTemplate(
          "Failed to parse %1% enum value: value of unexpected type got: %2%",
          GetEnumTypeName(),
          DebugDumpVar(var));
      return false;
    }

    bool succeeded = false;
    VisitCorrespondingPairs([&var_value, &succeeded, &result](
        EnumType candidate_value, const VarValueType& candidate_var_value) {
      if (var_value == candidate_var_value) {
        GOOGLE_SMART_CARD_CHECK(!succeeded);
        succeeded = true;
        *result = candidate_value;
      }
    });
    if (!succeeded) {
      *error_message = FormatBoostFormatTemplate(
          "Failed to parse %1% enum value: unknown value got: %2%",
          GetEnumTypeName(),
          DebugDumpVar(var));
    }
    return succeeded;
  }

  // Returns the textual name of the EnumType (used for displaying it in error
  // messages).
  //
  // The class consumer has to implement this method for the class
  // specialization that corresponds to this EnumType.
  static constexpr const char* GetEnumTypeName();

 private:
  // Generates a number of pairs: (C/C++ enum value, var value) and calls the
  // specified callback with each of them.
  //
  // The class consumer has to implement this method for the class
  // specialization that corresponds to this EnumType.
  //
  // The Callback will be the functor with the following signature:
  // void(EnumType, const VarValueType&).
  template <typename Callback>
  static void VisitCorrespondingPairs(Callback callback);
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_ENUM_CONVERTER_H_
