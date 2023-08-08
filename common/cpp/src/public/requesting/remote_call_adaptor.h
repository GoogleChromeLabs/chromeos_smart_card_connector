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

#ifndef GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_REQUESTING_REMOTE_CALL_ADAPTOR_H_
#define GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_REQUESTING_REMOTE_CALL_ADAPTOR_H_

#include <string>
#include <utility>

#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/requesting/async_request.h"
#include "common/cpp/src/public/requesting/remote_call_arguments_conversion.h"
#include "common/cpp/src/public/requesting/remote_call_message.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/requesting/requester.h"

namespace google_smart_card {

// This is a helper class that implements a remote function call interface on
// top of the specified requester.
//
// The remote function call request is parametrized with the function name and
// an array of its input arguments; the result of the remote function call
// request is expected to be an array of the returned values.
class RemoteCallAdaptor final {
 public:
  explicit RemoteCallAdaptor(Requester* requester);
  RemoteCallAdaptor(const RemoteCallAdaptor&) = delete;
  RemoteCallAdaptor& operator=(const RemoteCallAdaptor&) = delete;
  ~RemoteCallAdaptor();

  template <typename... Args>
  GenericRequestResult SyncCall(std::string function_name, Args&&... args) {
    return PerformSyncRequest(ConvertToRemoteCallRequestPayloadOrDie(
        std::move(function_name), std::forward<Args>(args)...));
  }

  template <typename... Args>
  GenericAsyncRequest AsyncCall(GenericAsyncRequestCallback callback,
                                std::string function_name,
                                Args&&... args) {
    return StartAsyncRequest(
        ConvertToRemoteCallRequestPayloadOrDie(std::move(function_name),
                                               std::forward<Args>(args)...),
        callback);
  }

  template <typename... Args>
  void AsyncCall(GenericAsyncRequest* async_request,
                 GenericAsyncRequestCallback callback,
                 std::string function_name,
                 Args&&... args) {
    StartAsyncRequest(
        ConvertToRemoteCallRequestPayloadOrDie(std::move(function_name),
                                               std::forward<Args>(args)...),
        callback, async_request);
  }

  template <typename... PayloadFields>
  static bool ExtractResultPayload(GenericRequestResult generic_request_result,
                                   std::string* error_message,
                                   PayloadFields*... payload_fields) {
    // TODO(#233): Receive the function name as a parameter, and use it in both
    // `error_message` here and in `RemoteCallArgumentsExtractor` below.
    if (!generic_request_result.is_successful()) {
      *error_message = generic_request_result.error_message();
      return false;
    }
    RemoteCallArgumentsExtractor extractor(
        "unknown_function", std::move(generic_request_result).TakePayload());
    extractor.Extract(payload_fields...);
    if (!extractor.success() && error_message)
      *error_message = extractor.error_message();
    return extractor.success();
  }

  template <typename PayloadType, typename... PayloadFields>
  static RequestResult<PayloadType> ConvertResultPayload(
      GenericRequestResult generic_request_result,
      PayloadType* payload_in_case_of_success,
      PayloadFields*... payload_fields) {
    std::string error_message;
    if (!ExtractResultPayload(std::move(generic_request_result), &error_message,
                              payload_fields...)) {
      return RequestResult<PayloadType>::CreateFailed(error_message);
    }
    return RequestResult<PayloadType>::CreateSuccessful(
        std::move(*payload_in_case_of_success));
  }

 private:
  GenericRequestResult PerformSyncRequest(RemoteCallRequestPayload payload);

  GenericAsyncRequest StartAsyncRequest(RemoteCallRequestPayload payload,
                                        GenericAsyncRequestCallback callback);

  Requester* const requester_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_CPP_SRC_PUBLIC_REQUESTING_REMOTE_CALL_ADAPTOR_H_
