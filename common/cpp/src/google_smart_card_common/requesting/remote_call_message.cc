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

#include <google_smart_card_common/requesting/remote_call_message.h>

#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/debug_dump.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

namespace google_smart_card {

namespace {

constexpr char kFunctionNameMessageField[] = "function_name";
constexpr char kFunctionArgumentsMessageField[] = "arguments";

}  // namespace

pp::Var MakeRemoteCallRequestPayload(const std::string& function_name,
                                     const pp::VarArray& arguments) {
  return VarDictBuilder()
      .Add(kFunctionNameMessageField, function_name)
      .Add(kFunctionArgumentsMessageField, arguments)
      .Result();
}

bool ParseRemoteCallRequestPayload(const pp::Var& request_payload,
                                   std::string* function_name,
                                   pp::VarArray* arguments) {
  std::string error_message;
  pp::VarDictionary request_payload_dict;
  if (!VarAs(request_payload, &request_payload_dict, &error_message))
    return false;
  return VarDictValuesExtractor(request_payload_dict)
      .Extract(kFunctionNameMessageField, function_name)
      .Extract(kFunctionArgumentsMessageField, arguments)
      .GetSuccessWithNoExtraKeysAllowed(&error_message);
}

std::string DebugDumpRemoteCallRequest(const std::string& function_name,
                                       const pp::VarArray& arguments) {
  std::string dumped_arguments = DebugDumpVar(arguments);
  GOOGLE_SMART_CARD_CHECK(!dumped_arguments.empty());
  GOOGLE_SMART_CARD_CHECK(dumped_arguments.front() == '[');
  GOOGLE_SMART_CARD_CHECK(dumped_arguments.back() == ']');
  dumped_arguments.front() = '(';
  dumped_arguments.back() = ')';
  return function_name + dumped_arguments;
}

}  // namespace google_smart_card
