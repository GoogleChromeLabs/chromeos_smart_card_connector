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

namespace google_smart_card {

RemoteCallAdaptor::RemoteCallAdaptor(Requester* requester)
    : requester_(requester) {
  GOOGLE_SMART_CARD_CHECK(requester_);
}

GenericRequestResult RemoteCallAdaptor::PerformSyncRequest(
    const std::string& function_name, const pp::VarArray& converted_arguments) {
  return requester_->PerformSyncRequest(MakeRemoteCallRequestPayload(
      function_name, converted_arguments));
}

GenericAsyncRequest RemoteCallAdaptor::StartAsyncRequest(
    const std::string& function_name,
    const pp::VarArray& converted_arguments,
    GenericAsyncRequestCallback callback) {
  return requester_->StartAsyncRequest(
      MakeRemoteCallRequestPayload(function_name, converted_arguments),
      callback);
}

void RemoteCallAdaptor::StartAsyncRequest(
    const std::string& function_name,
    const pp::VarArray& converted_arguments,
    GenericAsyncRequestCallback callback,
    GenericAsyncRequest* async_request) {
  requester_->StartAsyncRequest(
      MakeRemoteCallRequestPayload(function_name, converted_arguments),
      callback,
      async_request);
}

}  // namespace google_smart_card
