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

#include <string>

#include <google_smart_card_common/value_conversion.h>
#include <google_smart_card_common/value_debug_dumping.h>

namespace google_smart_card {

// Register the struct for conversions to/from `Value`.
template <>
StructValueDescriptor<RemoteCallRequestPayload>::Description
StructValueDescriptor<RemoteCallRequestPayload>::GetDescription() {
  // Note: Strings passed to WithField() below must match the keys in
  // //common/js/src/requesting/remote-call-message.js.
  return Describe("RemoteCallRequestPayload")
      .WithField(&RemoteCallRequestPayload::function_name, "function_name")
      .WithField(&RemoteCallRequestPayload::arguments, "arguments");
}

std::string RemoteCallRequestPayload::DebugDumpSanitized() const {
  std::string debug_dump = function_name;
  debug_dump += '(';
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (i > 0)
      debug_dump += ',';
    debug_dump += DebugDumpValueSanitized(arguments[i]);
  }
  debug_dump += ')';
  return debug_dump;
}

}  // namespace google_smart_card
