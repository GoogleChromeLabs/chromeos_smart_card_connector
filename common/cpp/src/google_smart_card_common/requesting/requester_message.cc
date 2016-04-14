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

#include <google_smart_card_common/requesting/requester_message.h>

#include <ppapi/cpp/var_dictionary.h>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/pp_var_utils/copying.h>
#include <google_smart_card_common/pp_var_utils/extraction.h>
#include <google_smart_card_common/pp_var_utils/operations.h>

const char kRequestMessageTypeSuffix[] = "::request";
const char kResponseMessageTypeSuffix[] = "::response";
const char kRequestIdMessageKey[] = "request_id";
const char kPayloadMessageKey[] = "payload";
const char kErrorMessageKey[] = "error";
const char kCanceledErrorMessage[] = "The request was canceled";

namespace google_smart_card {

std::string GetRequestMessageType(const std::string& name) {
  return name + kRequestMessageTypeSuffix;
}

std::string GetResponseMessageType(const std::string& name) {
  return name + kResponseMessageTypeSuffix;
}

namespace {

pp::VarDictionary MakeMessageDataWithRequestId(
    const pp::VarDictionary& message_data, RequestId request_id) {
  GOOGLE_SMART_CARD_CHECK(!message_data.HasKey(kRequestIdMessageKey));
  pp::VarDictionary result = ShallowCopyVar(message_data);
  AddVarDictValue(&result, kRequestIdMessageKey, request_id);
  return result;
}

}  // namespace

pp::Var MakeRequestMessageData(
    RequestId request_id, const pp::Var& payload) {
  pp::VarDictionary message_data;
  AddVarDictValue(&message_data, kPayloadMessageKey, payload);
  return MakeMessageDataWithRequestId(message_data, request_id);
}

namespace {

pp::Var MakeSucceededResponseMessageData(
    RequestId request_id, const pp::Var& response_payload) {
  pp::VarDictionary message_data;
  AddVarDictValue(&message_data, kPayloadMessageKey, response_payload);
  return MakeMessageDataWithRequestId(message_data, request_id);
}

pp::Var MakeFailedResponseMessageData(
    RequestId request_id, const std::string& error_message) {
  pp::VarDictionary message_data;
  AddVarDictValue(&message_data, kErrorMessageKey, error_message);
  return MakeMessageDataWithRequestId(message_data, request_id);
}

pp::Var MakeCanceledResponseMessageData(RequestId request_id) {
  return MakeFailedResponseMessageData(request_id, kCanceledErrorMessage);
}

}  // namespace

pp::Var MakeResponseMessageData(
    RequestId request_id, const GenericRequestResult& request_result) {
  switch (request_result.status()) {
    case RequestResultStatus::kSucceeded:
      return MakeSucceededResponseMessageData(
          request_id, request_result.payload());
    case RequestResultStatus::kFailed:
      return MakeFailedResponseMessageData(
          request_id, request_result.error_message());
    case RequestResultStatus::kCanceled:
      return MakeCanceledResponseMessageData(request_id);
    default:
      GOOGLE_SMART_CARD_NOTREACHED;
  }
}

bool ParseRequestMessageData(
    const pp::Var& message_data, RequestId* request_id, pp::Var* payload) {
  std::string error_message;
  pp::VarDictionary message_data_dict;
  if (!VarAs(message_data, &message_data_dict, &error_message))
    return false;
  return VarDictValuesExtractor(message_data_dict)
      .Extract(kRequestIdMessageKey, request_id)
      .Extract(kPayloadMessageKey, payload)
      .GetSuccessWithNoExtraKeysAllowed(&error_message);
}

bool ParseResponseMessageData(
    const pp::Var& message_data,
    RequestId* request_id,
    GenericRequestResult* request_result) {
  std::string error_message;
  pp::VarDictionary message_data_dict;
  if (!VarAs(message_data, &message_data_dict, &error_message))
    return false;
  if (GetVarDictSize(message_data_dict) != 2) {
    // There are some missing or some extra keys.
    return false;
  }
  if (!GetVarDictValueAs(
           message_data_dict,
           kRequestIdMessageKey,
           request_id,
           &error_message)) {
    return false;
  }
  pp::Var response_payload;
  if (GetVarDictValueAs(
          message_data_dict,
          kPayloadMessageKey,
          &response_payload,
          &error_message)) {
    *request_result = GenericRequestResult::CreateSuccessful(response_payload);
    return true;
  }
  std::string response_error_message;
  if (GetVarDictValueAs(
          message_data_dict,
          kErrorMessageKey,
          &response_error_message,
          &error_message)) {
    *request_result = GenericRequestResult::CreateFailed(
        response_error_message);
    return true;
  }
  return false;
}

}  // namespace google_smart_card
