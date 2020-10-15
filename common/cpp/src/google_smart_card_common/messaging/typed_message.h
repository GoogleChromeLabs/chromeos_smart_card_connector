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

// This file contains helper definitions for dealing with the "typed" Pepper
// messages.
//
// A "typed" Pepper message is a dictionary value with exactly two keys: one
// containing the message type (a string value), and another containing the
// message data (can be an arbitrary value).
//
// FIXME(emaxx): Add CHECKs for correspondence with the JavaScript side (looks
// like the data should always be a non-null Object).

#ifndef GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_H_
#define GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_H_

#include <string>

#include <ppapi/cpp/var.h>

namespace google_smart_card {

// Parses the typed message, splitting it into two components: the message type
// and the message data.
bool ParseTypedMessage(const pp::Var& message, std::string* type,
                       pp::Var* data);

// Creates a typed message having the specified message type and message data.
pp::Var MakeTypedMessage(const std::string& type, const pp::Var& data);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_MESSAGING_TYPED_MESSAGE_H_
