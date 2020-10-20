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

#ifndef GOOGLE_SMART_CARD_COMMON_MESSAGING_MESSAGE_LISTENER_H_
#define GOOGLE_SMART_CARD_COMMON_MESSAGING_MESSAGE_LISTENER_H_

#include <string>

#include <google_smart_card_common/value.h>

namespace google_smart_card {

// The abstract class for a Pepper message listener.
class MessageListener {
 public:
  virtual ~MessageListener() = default;

  // Called when a message is received.
  //
  // Returns whether the message was handled; in case it was not, the error is
  // returned via `error_message`.
  virtual bool OnMessageReceived(Value message,
                                 std::string* error_message = nullptr) = 0;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_MESSAGING_MESSAGE_LISTENER_H_
