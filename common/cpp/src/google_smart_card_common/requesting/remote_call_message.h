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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_MESSAGE_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_MESSAGE_H_

#include <string>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>

namespace google_smart_card {

// Constructs the message data payload of the remote call request, containing
// the specified function name and the array of the function arguments.
pp::Var MakeRemoteCallRequestPayload(
    const std::string& function_name, const pp::VarArray& arguments);

// Parses the message data payload of the remote call request, extracting the
// function name and the array of the function arguments.
bool ParseRemoteCallRequestPayload(
    const pp::Var& request_payload,
    std::string* function_name,
    pp::VarArray* arguments);

// Generates a human-readable debug dump of the remote call request.
std::string DebugDumpRemoteCallRequest(
    const std::string& function_name, const pp::VarArray& arguments);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_MESSAGE_H_
