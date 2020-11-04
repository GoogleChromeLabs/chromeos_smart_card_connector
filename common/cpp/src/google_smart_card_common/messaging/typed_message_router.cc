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
#include <string>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/messaging/typed_message.h>
#include <google_smart_card_common/value_conversion.h>

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

bool TypedMessageRouter::OnMessageReceived(Value message,
                                           std::string* error_message) {
  TypedMessage typed_message;
  std::string local_error_message;
  if (!ConvertFromValue(std::move(message), &typed_message,
                        &local_error_message)) {
    FormatPrintfTemplateAndSet(error_message, "Cannot parse typed message: %s",
                               local_error_message.c_str());
    return false;
  }

  TypedMessageListener* const listener = FindListenerByType(typed_message.type);
  if (!listener) {
    FormatPrintfTemplateAndSet(error_message, "Cannot find listener for %s",
                               typed_message.type.c_str());
    return false;
  }

  if (!listener->OnTypedMessageReceived(std::move(typed_message.data))) {
    // TODO: Receive `error_message` from the listener.
    return false;
  }
  return true;
}

TypedMessageListener* TypedMessageRouter::FindListenerByType(
    const std::string& message_type) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto route_map_iter = route_map_.find(message_type);
  if (route_map_iter == route_map_.end())
    return nullptr;
  return route_map_iter->second;
}

}  // namespace google_smart_card
