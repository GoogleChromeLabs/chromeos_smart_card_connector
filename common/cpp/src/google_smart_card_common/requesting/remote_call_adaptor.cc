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

#include <google_smart_card_common/requesting/remote_call_adaptor.h>

#include <utility>

#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/requesting/remote_call_message.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

namespace google_smart_card {

RemoteCallAdaptor::RemoteCallAdaptor(Requester* requester)
    : requester_(requester) {
  GOOGLE_SMART_CARD_CHECK(requester_);
}

RemoteCallAdaptor::~RemoteCallAdaptor() = default;

GenericRequestResult RemoteCallAdaptor::PerformSyncRequest(
    const std::string& function_name,
    const pp::VarArray& converted_arguments) {
  // TODO(#185): Create `Value` directly, without converting from `pp::Var`.
  return requester_->PerformSyncRequest(ConvertPpVarToValueOrDie(
      MakeRemoteCallRequestPayload(function_name, converted_arguments)));
}

GenericAsyncRequest RemoteCallAdaptor::StartAsyncRequest(
    const std::string& function_name,
    const pp::VarArray& converted_arguments,
    GenericAsyncRequestCallback callback) {
  // TODO(#185): Create `Value` directly, without converting from `pp::Var`.
  return requester_->StartAsyncRequest(
      ConvertPpVarToValueOrDie(
          MakeRemoteCallRequestPayload(function_name, converted_arguments)),
      callback);
}

void RemoteCallAdaptor::StartAsyncRequest(
    const std::string& function_name,
    const pp::VarArray& converted_arguments,
    GenericAsyncRequestCallback callback,
    GenericAsyncRequest* async_request) {
  // TODO(#185): Create `Value` directly, without converting from `pp::Var`.
  requester_->StartAsyncRequest(
      ConvertPpVarToValueOrDie(
          MakeRemoteCallRequestPayload(function_name, converted_arguments)),
      callback, async_request);
}

}  // namespace google_smart_card
