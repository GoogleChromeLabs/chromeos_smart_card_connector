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

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_ADAPTOR_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_ADAPTOR_H_

#include <string>

#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/construction.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/requesting/async_request.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/requesting/requester.h>
#include <google_smart_card_common/value_nacl_pp_var_conversion.h>

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
  GenericRequestResult SyncCall(const std::string& function_name,
                                const Args&... args) {
    return PerformSyncRequest(function_name, ConvertRequestArguments(args...));
  }

  template <typename... Args>
  GenericAsyncRequest AsyncCall(GenericAsyncRequestCallback callback,
                                const std::string& function_name,
                                const Args&... args) {
    return StartAsyncRequest(function_name, ConvertRequestArguments(args...),
                             callback);
  }

  template <typename... Args>
  void AsyncCall(GenericAsyncRequest* async_request,
                 GenericAsyncRequestCallback callback,
                 const std::string& function_name, const Args&... args) {
    StartAsyncRequest(function_name, ConvertRequestArguments(args...), callback,
                      async_request);
  }

  template <typename... PayloadFields>
  static bool ExtractResultPayload(
      const GenericRequestResult& generic_request_result,
      std::string* error_message, PayloadFields*... payload_fields) {
    // TODO(emaxx): Probably add details about the function call into the error
    // messages?
    if (!generic_request_result.is_successful()) {
      *error_message = generic_request_result.error_message();
      return false;
    }
    // TODO(#185): Parse `Value` diectly, without converting to `pp::Var`.
    pp::Var payload_var = ConvertValueToPpVar(generic_request_result.payload());
    pp::VarArray var_array;
    if (!VarAs(payload_var, &var_array, error_message)) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Failed to extract the response "
                                  << "payload items: " << *error_message;
    }
    if (!TryGetVarArrayItems(var_array, error_message, payload_fields...)) {
      GOOGLE_SMART_CARD_LOG_FATAL << "Failed to extract the response "
                                  << "payload items: " << *error_message;
    }
    return true;
  }

  template <typename PayloadType, typename... PayloadFields>
  static RequestResult<PayloadType> ConvertResultPayload(
      const GenericRequestResult& generic_request_result,
      PayloadType* payload_in_case_of_success,
      PayloadFields*... payload_fields) {
    std::string error_message;
    if (!ExtractResultPayload(generic_request_result, &error_message,
                              payload_fields...)) {
      return RequestResult<PayloadType>::CreateFailed(error_message);
    }
    return RequestResult<PayloadType>::CreateSuccessful(
        std::move(*payload_in_case_of_success));
  }

 private:
  template <typename... Args>
  static pp::VarArray ConvertRequestArguments(const Args&... args) {
    return MakeVarArray(args...);
  }

  GenericRequestResult PerformSyncRequest(
      const std::string& function_name,
      const pp::VarArray& converted_arguments);

  GenericAsyncRequest StartAsyncRequest(const std::string& function_name,
                                        const pp::VarArray& converted_arguments,
                                        GenericAsyncRequestCallback callback);

  void StartAsyncRequest(const std::string& function_name,
                         const pp::VarArray& converted_arguments,
                         GenericAsyncRequestCallback callback,
                         GenericAsyncRequest* async_request);

  Requester* const requester_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REMOTE_CALL_ADAPTOR_H_
