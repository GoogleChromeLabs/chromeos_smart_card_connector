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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_ARGUMENTS_CONVERSION_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_ARGUMENTS_CONVERSION_H_

#include <string>
#include <utility>
#include <vector>

#include "common/cpp/src/google_smart_card_common/logging/logging.h"
#include "common/cpp/src/google_smart_card_common/optional.h"
#include "common/cpp/src/google_smart_card_common/requesting/remote_call_message.h"
#include "common/cpp/src/google_smart_card_common/value.h"
#include "common/cpp/src/google_smart_card_common/value_conversion.h"

namespace google_smart_card {

///////////// Internal helpers ///////////////////

namespace internal {

void DieOnRemoteCallArgConversionError(const std::string& function_name,
                                       int argument_index,
                                       const std::string& error_message);

// Converts the argument into a `Value` and appends it into `payload`.
// Immediately crashes in case the conversion fails.
template <typename T>
void ConvertAndAppendRemoteCallArg(RemoteCallRequestPayload* payload,
                                   T&& argument) {
  std::string error_message;
  Value value;
  if (!ConvertToValue(std::forward<T>(argument), &value, &error_message)) {
    DieOnRemoteCallArgConversionError(
        payload->function_name, static_cast<int>(payload->arguments.size()),
        error_message);
  }
  payload->arguments.push_back(std::move(value));
}

// Below are three overloads for handling the special case of an `optional`
// argument. Overloads are covering several cases ((1) lvalue reference, (2)
// rvalue reference, (3) const reference) in order to prevent the overload above
// from being mistakenly triggered for an `optional` argument.

template <typename T>
void ConvertAndAppendRemoteCallArg(RemoteCallRequestPayload* payload,
                                   optional<T>& arg) {
  if (arg)
    ConvertAndAppendRemoteCallArg(payload, arg.value());
  else
    payload->arguments.emplace_back();
}

template <typename T>
void ConvertAndAppendRemoteCallArg(RemoteCallRequestPayload* payload,
                                   optional<T>&& arg) {
  if (arg)
    ConvertAndAppendRemoteCallArg(payload, std::move(arg.value()));
  else
    payload->arguments.emplace_back();
}

template <typename T>
void ConvertAndAppendRemoteCallArg(RemoteCallRequestPayload* payload,
                                   const optional<T>& arg) {
  if (arg)
    ConvertAndAppendRemoteCallArg(payload, arg.value());
  else
    payload->arguments.emplace_back();
}

// Below are two overloads that convert a sequence of arguments into `payload`.
// The first overload handles the base case when there's no argument, and the
// second overload works in case there's at least one argument.

inline void FillRemoteCallRequestArgs(RemoteCallRequestPayload* /*payload*/) {}

template <typename FirstArg, typename... Args>
void FillRemoteCallRequestArgs(RemoteCallRequestPayload* payload,
                               FirstArg&& first_arg,
                               Args&&... args) {
  // Convert the first argument.
  ConvertAndAppendRemoteCallArg(payload, std::forward<FirstArg>(first_arg));
  // Recursively process the subsequent arguments.
  FillRemoteCallRequestArgs(payload, std::forward<Args>(args)...);
}

}  // namespace internal

///////////// Public interface ///////////////////

// Creates `RemoteCallRequestPayload` with the given `function_name` and
// converted `args`. Immediately crashes the program if the conversion fails.
// Note: null `optional` arguments are converted into null `Value`s.
template <typename... Args>
RemoteCallRequestPayload ConvertToRemoteCallRequestPayloadOrDie(
    std::string function_name,
    Args&&... args) {
  RemoteCallRequestPayload payload;
  payload.function_name = std::move(function_name);
  internal::FillRemoteCallRequestArgs(&payload, std::forward<Args>(args)...);
  return payload;
}

// Helper that allows to convert the given array of `Value` arguments into C++
// objects.
class RemoteCallArgumentsExtractor final {
 public:
  RemoteCallArgumentsExtractor(std::string title,
                               std::vector<Value> argument_values);
  // Same as above, but attempts to convert the given `Value` into `vector`.
  RemoteCallArgumentsExtractor(std::string title, Value arguments_as_value);
  RemoteCallArgumentsExtractor(const RemoteCallArgumentsExtractor&) = delete;
  RemoteCallArgumentsExtractor& operator=(const RemoteCallArgumentsExtractor&) =
      delete;
  ~RemoteCallArgumentsExtractor();

  size_t argument_count() const { return argument_values_.size(); }
  bool success() const { return success_; }
  const std::string& error_message() const { return error_message_; }

  // Two overloads that implement a variadic template that extracts and converts
  // `Value`s into the given C++ objects (passed by pointers).
  template <typename FirstArgPtr, typename... ArgPtrs>
  void Extract(FirstArgPtr first_arg_ptr, ArgPtrs... arg_ptrs) {
    // Note: The check is performed here with the overall number of arguments,
    // rather than in `ExtractArgument()`, in order to put the real number into
    // the error log.
    VerifySufficientCount(sizeof...(arg_ptrs) + 1);
    ExtractArgument(first_arg_ptr);
    Extract(arg_ptrs...);
  }
  void Extract();

  // Finishes the conversion by checking that no unconverted argument is left.
  // Returns the result of `success()`.
  bool Finish();

 private:
  // Attempts to convert the value with the current index into the given `arg`.
  template <typename Arg>
  void ExtractArgument(Arg* arg) {
    if (!success_)
      return;
    if (ConvertFromValue(std::move(argument_values_[current_argument_index_]),
                         arg, &error_message_)) {
      ++current_argument_index_;
      return;
    }
    HandleArgumentConversionError();
  }

  // Specialized version of the overload above that supports converting a null
  // `Value` into a null `optional`.
  template <typename Arg>
  void ExtractArgument(optional<Arg>* arg) {
    if (!success_)
      return;
    if (argument_values_[current_argument_index_].is_null()) {
      arg->reset();
      ++current_argument_index_;
      return;
    }
    *arg = Arg();
    ExtractArgument(&arg->value());
  }

  void HandleArgumentConversionError();
  void VerifySufficientCount(int arguments_to_convert);
  void VerifyNothingLeft();

  const std::string title_;
  std::vector<Value> argument_values_;
  size_t current_argument_index_ = 0;
  bool success_ = true;
  std::string error_message_;
};

// Shortcut method for converting the given list of arguments via
// `RemoteCallArgumentsExtractor`.
template <typename... Args>
bool ExtractRemoteCallArguments(std::string function_name,
                                std::vector<Value> argument_values,
                                std::string* error_message,
                                Args... args) {
  RemoteCallArgumentsExtractor extractor(std::move(function_name),
                                         std::move(argument_values));
  extractor.Extract(args...);
  if (extractor.Finish())
    return true;
  if (error_message)
    *error_message = extractor.error_message();
  return false;
}

// Shortcut method for converting the given list of arguments via
// `RemoteCallArgumentsExtractor`, with immediately crashing the program on
// failures.
template <typename... Args>
void ExtractRemoteCallArgumentsOrDie(std::string function_name,
                                     std::vector<Value> argument_values,
                                     Args... args) {
  std::string error_message;
  if (!ExtractRemoteCallArguments(std::move(function_name),
                                  std::move(argument_values), &error_message,
                                  args...)) {
    GOOGLE_SMART_CARD_LOG_FATAL << error_message;
  }
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_ARGUMENTS_CONVERSION_H_
