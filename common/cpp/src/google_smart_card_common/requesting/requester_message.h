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

// This file contains various types and functions for creating and parsing
// messages for requests and for request responses.
//
// See google_smart_card_common/messaging/typed_message.h file for the
// description of notion of "message type" and "message data".
//
// Generally, the requester message data consists of request identifier and
// payload data. The response message data consists of request identifier and
// either payload data or error message.

#ifndef GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUESTER_MESSAGE_H_
#define GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUESTER_MESSAGE_H_

#include <string>

#include <ppapi/cpp/var.h>

#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_result.h>

namespace google_smart_card {

// Returns the message type for the requests for the requester with the
// specified name.
std::string GetRequestMessageType(const std::string& name);

// Returns the message type for the requests responses for the requester with
// the specified name.
std::string GetResponseMessageType(const std::string& name);

// Constructs message data that contains the request data.
pp::Var MakeRequestMessageData(RequestId request_id, const pp::Var& payload);

// Constructs message data that contains the response data.
//
// Note that the if the response has the RequestResultStatus::kCanceled state,
// then for simplicity reasons it will be serialized as a failed response.
pp::Var MakeResponseMessageData(RequestId request_id,
                                const GenericRequestResult& request_result);

// Parses the request message data, extracting the request identifier and the
// request data from it.
bool ParseRequestMessageData(const pp::Var& message_data, RequestId* request_id,
                             pp::Var* payload);

// Parses the request response message data, extracting the request identifier
// and the request result information from it.
//
// Note that the only possible result states returned are
// RequestResultStatus::kSucceeded and RequestResultStatus::kFailed.
bool ParseResponseMessageData(const pp::Var& message_data,
                              RequestId* request_id,
                              GenericRequestResult* request_result);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUESTER_MESSAGE_H_
