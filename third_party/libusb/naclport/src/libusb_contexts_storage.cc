// Copyright 2016 Google Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include "libusb_contexts_storage.h"

namespace google_smart_card {

LibusbContextsStorage::LibusbContextsStorage() = default;

LibusbContextsStorage::~LibusbContextsStorage() = default;

std::shared_ptr<libusb_context> LibusbContextsStorage::CreateContext() {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto result = std::make_shared<libusb_context>();
  GOOGLE_SMART_CARD_CHECK(mapping_.emplace(result.get(), result).second);
  return result;
}

void LibusbContextsStorage::DestroyContext(const libusb_context* context) {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto iter = mapping_.find(context);
  GOOGLE_SMART_CARD_CHECK(iter != mapping_.end());
  mapping_.erase(iter);
}

std::shared_ptr<libusb_context> LibusbContextsStorage::FindContextByAddress(
    const libusb_context* context) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto iter = mapping_.find(context);
  GOOGLE_SMART_CARD_CHECK(iter != mapping_.end());

  return iter->second;
}

}  // namespace google_smart_card
