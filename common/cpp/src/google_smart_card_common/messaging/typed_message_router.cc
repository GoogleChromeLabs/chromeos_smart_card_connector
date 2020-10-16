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

#include <google_smart_card_common/messaging/typed_message_router.h>

#include <algorithm>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>

namespace google_smart_card {

TypedMessageRouter::TypedMessageRouter() = default;

TypedMessageRouter::~TypedMessageRouter() = default;

void TypedMessageRouter::AddRoute(TypedMessageListener* listener) {
  const std::unique_lock<std::mutex> lock(mutex_);

  const bool is_new_route_added =
      route_map_.emplace(listener->GetListenedMessageType(), listener).second;
  GOOGLE_SMART_CARD_CHECK(is_new_route_added);
}

void TypedMessageRouter::RemoveRoute(TypedMessageListener* listener) {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto route_map_iter =
      route_map_.find(listener->GetListenedMessageType());
  GOOGLE_SMART_CARD_CHECK(route_map_iter != route_map_.end());
  GOOGLE_SMART_CARD_CHECK(route_map_iter->second == listener);
  route_map_.erase(route_map_iter);
}

bool TypedMessageRouter::OnMessageReceived(const pp::Var& message) {
  std::string error_message;
  TypedMessage typed_message;
  if (!VarAs(message, &typed_message, &error_message)) return false;

  TypedMessageListener* const listener = FindListenerByType(typed_message.type);
  if (!listener) return false;

  return listener->OnTypedMessageReceived(typed_message.data);
}

TypedMessageListener* TypedMessageRouter::FindListenerByType(
    const std::string& message_type) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto route_map_iter = route_map_.find(message_type);
  if (route_map_iter == route_map_.end()) return nullptr;
  return route_map_iter->second;
}

}  // namespace google_smart_card
