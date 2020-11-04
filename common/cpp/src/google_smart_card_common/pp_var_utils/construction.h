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

// This file contains various helper functions for constructing the Pepper
// values (pp::Var type and its descendant types).

#ifndef GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_CONSTRUCTION_H_
#define GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_CONSTRUCTION_H_

#ifndef __native_client__
#error "This file should only be used in Native Client builds"
#endif  // __native_client__

#include <stdint.h>

#include <string>
#include <type_traits>
#include <vector>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>

namespace google_smart_card {

//
// Following there is a group of functions that convert the passed argument into
// the pp::Var value.
//
// This provides a uniform interface for performing such conversions (comparing
// to the somewhat limited set of pp::Var constructors).
//
// Note that the consumers may provide additional overloads for supporting the
// custom types; this would automatically add a support for them into most of
// the other functions defined in this file.
//

template <typename T>
inline pp::Var MakeVar(const T& value) {
  return pp::Var(value);
}

pp::Var MakeVar(unsigned value);

pp::Var MakeVar(long value);

pp::Var MakeVar(unsigned long value);

pp::Var MakeVar(int64_t value);

pp::Var MakeVar(uint64_t value);

// Note that this function throws a fatal error if some characters of the string
// are not representable inside Pepper values; in order to handle such strings,
// the CleanupStringForVar function should be used.
pp::Var MakeVar(const std::string& value);

template <typename T>
inline pp::Var MakeVar(const optional<T>& value) {
  if (!value)
    return pp::Var();
  return MakeVar(*value);
}

template <typename T>
inline pp::Var MakeVar(const std::vector<T>& value) {
  pp::VarArray result;
  result.SetLength(value.size());
  for (size_t index = 0; index < value.size(); ++index)
    GOOGLE_SMART_CARD_CHECK(result.Set(index, MakeVar(value[index])));
  return result;
}

// Creates an array buffer.
pp::Var MakeVar(const std::vector<uint8_t>& value);

// Returns a string in which all characters that cannot be represented in a
// Pepper value are replaced with a placeholder.
std::string CleanupStringForVar(const std::string& string);

//
// Constructs a Pepper array buffer from the given data bytes.
//

pp::VarArrayBuffer MakeVarArrayBuffer(const std::vector<uint8_t>& data);

pp::VarArrayBuffer MakeVarArrayBuffer(const void* data, size_t size);

namespace internal {

inline void FillVarArray(pp::VarArray* /*var*/,
                         uint32_t /*current_item_index*/) {}

template <typename Arg, typename... Args>
inline void FillVarArray(pp::VarArray* var,
                         uint32_t current_item_index,
                         const Arg& arg,
                         const Args&... args) {
  GOOGLE_SMART_CARD_CHECK(var->Set(current_item_index, MakeVar(arg)));
  FillVarArray(var, current_item_index + 1, args...);
}

}  // namespace internal

// Constructs a Pepper array from the list of values of any supported type.
template <typename... Args>
inline pp::VarArray MakeVarArray(const Args&... args) {
  pp::VarArray result;
  internal::FillVarArray(&result, 0, args...);
  return result;
}

// Builds a pp::VarDictionary value, filling it with the specified items.
//
// A typical usage example:
//    VarDictBuilder()
//        .Add("key_1", value_1)
//        .Add("key_2", value_2)
//        .Result();
class VarDictBuilder final {
 public:
  VarDictBuilder();
  VarDictBuilder(const VarDictBuilder&) = delete;
  VarDictBuilder& operator=(const VarDictBuilder&) = delete;
  ~VarDictBuilder();

  template <typename T>
  VarDictBuilder& Add(const std::string& key, const T& value) {
    GOOGLE_SMART_CARD_CHECK(!dict_.HasKey(key));
    GOOGLE_SMART_CARD_CHECK(dict_.Set(key, MakeVar(value)));
    return *this;
  }

  pp::VarDictionary Result() const { return dict_; }

 private:
  pp::VarDictionary dict_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_PP_VAR_UTILS_CONSTRUCTION_H_
