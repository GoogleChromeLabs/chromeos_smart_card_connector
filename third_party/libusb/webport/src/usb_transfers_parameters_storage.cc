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

#include "third_party/libusb/webport/src/usb_transfers_parameters_storage.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <memory>
#include <utility>

#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/requesting/async_request.h"
#include "common/cpp/src/public/requesting/remote_call_async_request.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "third_party/libusb/webport/src/libusb_js_proxy_data_model.h"
#include "third_party/libusb/webport/src/usb_transfer_destination.h"

namespace google_smart_card {

template <typename Key>
UsbTransfersParametersStorage::Item* UsbTransfersParametersStorage::FindItem(
    const Key& mapping_key,
    const std::map<Key, Item*>& mapping) const {
  const auto iter = mapping.find(mapping_key);
  if (iter == mapping.end()) {
    return nullptr;
  }
  Item* const found = iter->second;
  GOOGLE_SMART_CARD_CHECK(found);
  return found;
}

template <typename Key>
UsbTransfersParametersStorage::Item* UsbTransfersParametersStorage::GetFifoItem(
    const Key& mapping_key,
    const std::map<Key, std::deque<Item*>>& mapping) const {
  const auto iter = mapping.find(mapping_key);
  if (iter == mapping.end()) {
    return nullptr;
  }
  const std::deque<Item*> items = iter->second;
  GOOGLE_SMART_CARD_CHECK(!items.empty());
  Item* const chosen = items.front();
  GOOGLE_SMART_CARD_CHECK(chosen);
  return chosen;
}

template <typename Key>
bool UsbTransfersParametersStorage::RemoveItemFromMappedQueue(
    Item* item,
    const Key& mapping_key,
    std::map<Key, std::deque<Item*>>& mapping) {
  const auto map_iter = mapping.find(mapping_key);
  if (map_iter == mapping.end()) {
    return false;
  }
  std::deque<Item*>& items = map_iter->second;
  GOOGLE_SMART_CARD_CHECK(!items.empty());
  const auto item_iter = std::find(items.begin(), items.end(), item);
  if (item_iter == items.end()) {
    return false;
  }
  items.erase(item_iter);
  if (items.empty()) {
    mapping.erase(map_iter);
  }
  return true;
}

UsbTransfersParametersStorage::UsbTransfersParametersStorage() = default;

UsbTransfersParametersStorage::~UsbTransfersParametersStorage() = default;

bool UsbTransfersParametersStorage::empty() const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return async_request_state_mapping_.empty();
}

void UsbTransfersParametersStorage::Add(std::unique_ptr<Item> item) {
  GOOGLE_SMART_CARD_CHECK(item);
  GOOGLE_SMART_CARD_CHECK(item->prepared_js_call);

  const std::unique_lock<std::mutex> lock(mutex_);

  Item* const item_ptr = item.get();
  const Info& info = item_ptr->info;
  GOOGLE_SMART_CARD_CHECK(items_.emplace(item_ptr, std::move(item)).second);
  // `item` should not be used after this point.

  GOOGLE_SMART_CARD_CHECK(async_request_state_mapping_
                              .emplace(info.async_request_state.get(), item_ptr)
                              .second);
  GOOGLE_SMART_CARD_CHECK(
      async_libusb_transfer_mapping_.emplace(info.transfer, item_ptr).second);
  async_destination_mapping_[info.transfer_destination].push_back(item_ptr);
  timeout_mapping_[info.timeout].push_back(item_ptr);
  transfers_with_prepared_js_call_[info.transfer_destination].push_back(
      item_ptr);
}

void UsbTransfersParametersStorage::Add(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination,
    libusb_transfer* transfer,
    RemoteCallAsyncRequest prepared_js_call,
    const std::chrono::time_point<std::chrono::steady_clock>& timeout) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);
  GOOGLE_SMART_CARD_CHECK(transfer);
  auto stored_item = MakeUnique<Item>();
  stored_item->info.async_request_state = async_request_state;
  stored_item->info.transfer_destination = transfer_destination;
  stored_item->info.transfer = transfer;
  stored_item->info.timeout = timeout;
  stored_item->prepared_js_call = std::move(prepared_js_call);
  return Add(std::move(stored_item));
}

bool UsbTransfersParametersStorage::ContainsWithAsyncRequestState(
    const TransferAsyncRequestState* async_request_state) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return FindItem(async_request_state, async_request_state_mapping_) != nullptr;
}

bool UsbTransfersParametersStorage::ContainsAsyncWithDestination(
    const UsbTransferDestination& transfer_destination) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return GetFifoItem(transfer_destination, async_destination_mapping_) !=
         nullptr;
}

bool UsbTransfersParametersStorage::ContainsAsyncWithLibusbTransfer(
    const libusb_transfer* transfer) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return FindItem(transfer, async_libusb_transfer_mapping_) != nullptr;
}

UsbTransfersParametersStorage::Info
UsbTransfersParametersStorage::GetByAsyncRequestState(
    const TransferAsyncRequestState* async_request_state) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  const Item* const found =
      FindItem(async_request_state, async_request_state_mapping_);
  GOOGLE_SMART_CARD_CHECK(found);
  return found->info;
}

UsbTransfersParametersStorage::Info
UsbTransfersParametersStorage::GetAsyncByDestination(
    const UsbTransferDestination& transfer_destination) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  const Item* const found =
      GetFifoItem(transfer_destination, async_destination_mapping_);
  GOOGLE_SMART_CARD_CHECK(found);
  return found->info;
}

UsbTransfersParametersStorage::Info
UsbTransfersParametersStorage::GetAsyncByLibusbTransfer(
    const libusb_transfer* transfer) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  const Item* const found = FindItem(transfer, async_libusb_transfer_mapping_);
  GOOGLE_SMART_CARD_CHECK(found);
  return found->info;
}

UsbTransfersParametersStorage::Info
UsbTransfersParametersStorage::GetWithMinTimeout() const {
  const std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(!timeout_mapping_.empty());
  const auto min_timeout = timeout_mapping_.begin()->first;
  const Item* const found = GetFifoItem(min_timeout, timeout_mapping_);
  GOOGLE_SMART_CARD_CHECK(found);
  return found->info;
}

optional<RemoteCallAsyncRequest>
UsbTransfersParametersStorage::ExtractPreparedJsCall(
    const UsbTransferDestination& transfer_destination) {
  const std::unique_lock<std::mutex> lock(mutex_);

  Item* const item =
      GetFifoItem(transfer_destination, transfers_with_prepared_js_call_);
  if (!item) {
    return {};
  }
  GOOGLE_SMART_CARD_CHECK(item->prepared_js_call);

  RemoveItemFromMappedQueue(item, transfer_destination,
                            transfers_with_prepared_js_call_);
  RemoteCallAsyncRequest result = std::move(*item->prepared_js_call);
  item->prepared_js_call.reset();
  // TODO: Drop `std::move()` once NaCl build support is removed.
  return std::move(result);
}

void UsbTransfersParametersStorage::Remove(Item* item) {
  GOOGLE_SMART_CARD_CHECK(item);
  const Info& info = item->info;

  const std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(
      async_request_state_mapping_.erase(info.async_request_state.get()));
  GOOGLE_SMART_CARD_CHECK(async_libusb_transfer_mapping_.erase(info.transfer));
  GOOGLE_SMART_CARD_CHECK(RemoveItemFromMappedQueue(
      item, info.transfer_destination, async_destination_mapping_));
  GOOGLE_SMART_CARD_CHECK(
      RemoveItemFromMappedQueue(item, info.timeout, timeout_mapping_));
  RemoveItemFromMappedQueue(item, info.transfer_destination,
                            transfers_with_prepared_js_call_);

  // Remove from `items_`. This must be the last operation with `item`.
  GOOGLE_SMART_CARD_CHECK(items_.erase(item));
  // `item` must not be used after this point.
}

void UsbTransfersParametersStorage::RemoveByAsyncRequestState(
    const TransferAsyncRequestState* async_request_state) {
  Remove(FindItem(async_request_state, async_request_state_mapping_));
}

}  // namespace google_smart_card
