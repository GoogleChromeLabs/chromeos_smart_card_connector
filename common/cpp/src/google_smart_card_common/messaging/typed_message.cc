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

#include <google_smart_card_common/messaging/typed_message.h>

#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

// Register `TypedMessage` for conversions to/from `Value`.
template <>
StructValueDescriptor<TypedMessage>::Description
StructValueDescriptor<TypedMessage>::GetDescription() {
  // Note: Strings passed to WithField() below must match the keys in
  // //common/js/src/messaging/typed-message.js.
  return Describe("TypedMessage")
      .WithField(&TypedMessage::type, "type")
      .WithField(&TypedMessage::data, "data");
}

TypedMessage::TypedMessage() = default;

TypedMessage::TypedMessage(TypedMessage&&) = default;

TypedMessage& TypedMessage::operator=(TypedMessage&&) = default;

TypedMessage::~TypedMessage() = default;

}  // namespace google_smart_card
