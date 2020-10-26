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

// This file contains helper definitions for request messages and for request
// response messages.
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

#include <google_smart_card_common/requesting/request_id.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/value.h>

namespace google_smart_card {

// Returns the message type for the requests for the requester with the
// specified name.
std::string GetRequestMessageType(const std::string& name);

// Returns the message type for the requests responses for the requester with
// the specified name.
std::string GetResponseMessageType(const std::string& name);

// Represents the contents of the `TypedMessage::data` field for "request"
// messages.
//
// Example usage scenario: Suppose the C++ code wants to make a "say_hello"
// request to the JavaScript side. In that case, the C++ code would generate a
// typed message with the following contents:
//   type = "say_hello::request"
//   data = serialized RequestMessageData structure with the following fields:
//     request_id = 123
//     payload = "Hello request from C++"
// This typed message would then be sent to the JS side, which, would respond
// with a typed message like this:
//   type = "say_hello::response"
//   data = serialized ResponseMessageData structure with the following fields:
//     request_id = 123
//     payload = "Hello response from JS"
struct RequestMessageData {
  // Unique identifier of the request; is used in order to correlate response
  // messages (see `ResponseMessageData`) with the requests.
  //
  // Note that this field must be unique among all requests with the same
  // `name`; in general, requests with different `name` can use overlapping IDs.
  RequestId request_id;
  // The request payload, represented as a generic `Value` object. Contents of
  // this field are specific to a particular type of request.
  Value payload;
};

// Represents the contents of the `TypedMessage::data` field for "response"
// messages.
//
// See the documentation above for more details.
struct ResponseMessageData {
  // Converts the `GenericRequestResult` object into `ResponseMessageData`.
  static ResponseMessageData CreateFromRequestResult(
      RequestId request_id, GenericRequestResult request_result);
  // Creates a `GenericRequestResult` object from the `payload`/`error_message`
  // fields. Returns false in case |this| is invalid (it's expected that exactly
  // one of {`payload`, `error_message`} are non-empty).
  // Note: this is a destructive operation - the fields are moved into the
  // resulting object.
  bool ExtractRequestResult(GenericRequestResult* request_result);

  // Identifier of the request. Should be equal to the
  // `RequestMessageData::request_id` field of the correspond request.
  RequestId request_id = -1;
  // The response payload, represented as a generic `Value` object, or a null in
  // case the request failed. Contents of the value stored here are specific to
  // a particular type of request.
  optional<Value> payload;
  // The error message, in case the request failed, or a null otherwise.
  optional<std::string> error_message;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_REQUESTING_REQUESTER_MESSAGE_H_
