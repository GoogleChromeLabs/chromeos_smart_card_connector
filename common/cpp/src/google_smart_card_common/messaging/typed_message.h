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

// This file contains helper definitions for dealing with the "typed" messages.
//
// A "typed" message is a pair of the message type (a string value) and the
// message data (can be an arbitrary value).
//
// FIXME(emaxx): Investigate whether there should be CHECKs for correspondence
// with the JavaScript side (looks like the data should always be a non-null
// Object).

#ifndef GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_H_
#define GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_H_

#include <string>

#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

// A typed message is a pair of message type and message data.
//
// It's intended to be used for sending/receiving information through generic
// data channels, e.g., when communicating to JavaScript code. The type field
// determines the recipient of the associated data; the recipient should know
// how to interpret the data.
//
// For example, our C++ logging code creates the following typed message when
// emitting a log:
//   type = "log_message"
//   data = {"log-level": ..., "text": ...}
// This typed message is then packed into a single variable that is sent to
// the JavaScript code:
//   {"type": "log_message", "data": {"log-level": ..., "text": ...}}
// On the JavaScript side, the message channel listener extracts the value of
// the "type" property, finds the handler (service) that has been registered for
// the "log_message" type, and passes the "data" property to it. The latter does
// the intended operation, after parsing the "log-level" and "text" properties.
struct TypedMessage {
  TypedMessage();
  TypedMessage(TypedMessage&&);
  TypedMessage& operator=(TypedMessage&&);
  ~TypedMessage();

  std::string type;
  Value data;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_H_
