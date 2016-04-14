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

#include <google_smart_card_common/messaging/typed_message.h>

#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>

const char kTypeMessageKey[] = "type";
const char kDataMessageKey[] = "data";

namespace google_smart_card {

bool ParseTypedMessage(
    const pp::Var& message, std::string* type, pp::Var* data) {
  std::string error_message;
  pp::VarDictionary message_dict;
  if (!VarAs(message, &message_dict, &error_message))
    return false;
  return VarDictValuesExtractor(message_dict)
      .Extract(kTypeMessageKey, type)
      .Extract(kDataMessageKey, data)
      .GetSuccessWithNoExtraKeysAllowed(&error_message);
}

pp::Var MakeTypedMessage(const std::string& type, const pp::Var& data) {
  return VarDictBuilder()
      .Add(kTypeMessageKey, type).Add(kDataMessageKey, data).Result();
}

}  // namespace google_smart_card
