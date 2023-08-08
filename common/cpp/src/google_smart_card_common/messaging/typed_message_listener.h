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

#ifndef GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_LISTENER_H_
#define GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_LISTENER_H_

#include <string>

#include "common/cpp/src/google_smart_card_common/value.h"

namespace google_smart_card {

// The abstract class for a listener of Pepper messages having a specific type.
//
// For the usage details please see the TypedMessageRouter class.
class TypedMessageListener {
 public:
  virtual ~TypedMessageListener() = default;

  // The type of the Pepper messages that have to be listened.
  //
  // Note: it is assumed that this method always returns the same value for the
  // same TypedMessageListener object.
  virtual std::string GetListenedMessageType() const = 0;

  // Called when a message of the requested type is received. The data argument
  // contains the whole message contents except its type key (for the details
  // please refer to the typed_message.h file).
  //
  // Returns whether the message was handled.
  virtual bool OnTypedMessageReceived(Value data) = 0;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_LISTENER_H_
