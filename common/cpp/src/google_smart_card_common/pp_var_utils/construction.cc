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

#include <google_smart_card_common/pp_var_utils/construction.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <limits>

#include <google_smart_card_common/numeric_conversions.h>

namespace google_smart_card {

namespace {

template <typename T>
pp::Var MakeVarFromInteger(T value) {
  if (CompareIntegers(std::numeric_limits<int32_t>::min(), value) <= 0 &&
      CompareIntegers(std::numeric_limits<int32_t>::max(), value) >= 0) {
    return static_cast<int32_t>(value);
  }
  std::string error_message;
  double double_value;
  if (!CastIntegerToDouble(value, &double_value, &error_message))
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  return double_value;
}

bool IsCharValidForVar(char character) {
  // This is probably a pessimisation, as probably some other characters can be
  // accepted in Pepper values, but this should be a simple and reliable subset.
  return std::isprint(static_cast<unsigned char>(character));
}

bool IsStringValidForVar(const std::string& string) {
  return std::all_of(string.begin(), string.end(), IsCharValidForVar);
}

}  // namespace

pp::Var MakeVar(unsigned value) {
  return MakeVarFromInteger(value);
}

pp::Var MakeVar(long value) {
  return MakeVarFromInteger(value);
}

pp::Var MakeVar(unsigned long value) {
  return MakeVarFromInteger(value);
}

pp::Var MakeVar(int64_t value) {
  return MakeVarFromInteger(value);
}

pp::Var MakeVar(uint64_t value) {
  return MakeVarFromInteger(value);
}

pp::Var MakeVar(const std::string& value) {
  GOOGLE_SMART_CARD_CHECK(IsStringValidForVar(value));
  return value;
}

std::string CleanupStringForVar(const std::string& string) {
  const char kPlaceholder = ' ';
  std::string result = string;
  std::replace_if(
      result.begin(),
      result.end(),
      std::not1(std::ptr_fun(IsCharValidForVar)),
      kPlaceholder);
  return result;
}

pp::VarArrayBuffer MakeVarArrayBuffer(const std::vector<uint8_t>& data) {
  if (data.empty())
    return pp::VarArrayBuffer();
  return MakeVarArrayBuffer(&data[0], data.size());
}

pp::VarArrayBuffer MakeVarArrayBuffer(const void* data, size_t size) {
  pp::VarArrayBuffer result(size);
  std::memcpy(result.Map(), data, size);
  result.Unmap();
  return result;
}

}  // namespace google_smart_card
