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

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>
#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/value.h>
#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

///////////// Internal helpers ///////////////////

namespace internal {

extern const char kRemoteCallArgumentConversionError[];

// Converts the argument into a `Value` and appends it into `payload`.
// Immediately crashes in case the conversion fails.
template <typename T>
void ConvertAndAppendRemoteCallArg(RemoteCallRequestPayload* payload,
                                   T&& argument) {
  std::string error_message;
  Value value;
  if (!ConvertToValue(std::forward<T>(argument), &value, &error_message)) {
    GOOGLE_SMART_CARD_LOG_FATAL << FormatPrintfTemplate(
        kRemoteCallArgumentConversionError,
        static_cast<int>(payload->arguments.size()),
        payload->function_name.c_str(), error_message.c_str());
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
                               FirstArg&& first_arg, Args&&... args) {
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
    std::string function_name, Args&&... args) {
  RemoteCallRequestPayload payload;
  payload.function_name = std::move(function_name);
  internal::FillRemoteCallRequestArgs(&payload, std::forward<Args>(args)...);
  return payload;
}

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_ARGUMENTS_CONVERSION_H_
