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

#include "usb_transfers_parameters_storage.h"

#include "common/cpp/src/google_smart_card_common/logging/logging.h"

namespace google_smart_card {

UsbTransfersParametersStorage::UsbTransfersParametersStorage() = default;

UsbTransfersParametersStorage::~UsbTransfersParametersStorage() = default;

UsbTransfersParametersStorage::Item::Item() : transfer(nullptr) {}

UsbTransfersParametersStorage::Item::Item(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination,
    libusb_transfer* transfer,
    const std::chrono::time_point<std::chrono::high_resolution_clock>& timeout)
    : async_request_state(async_request_state),
      transfer_destination(transfer_destination),
      transfer(transfer),
      timeout(timeout) {}

bool UsbTransfersParametersStorage::empty() const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return async_request_state_mapping_.empty();
}

void UsbTransfersParametersStorage::Add(const Item& item) {
  const std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(
      async_request_state_mapping_.emplace(item.async_request_state.get(), item)
          .second);

  if (item.transfer) {
    GOOGLE_SMART_CARD_CHECK(
        async_libusb_transfer_mapping_.emplace(item.transfer, item).second);
    GOOGLE_SMART_CARD_CHECK(
        async_destination_mapping_[item.transfer_destination]
            .insert(item.async_request_state.get())
            .second);
  }

  GOOGLE_SMART_CARD_CHECK(timeout_mapping_[item.timeout]
                              .insert(item.async_request_state.get())
                              .second);
}

void UsbTransfersParametersStorage::Add(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination,
    libusb_transfer* transfer,
    const std::chrono::time_point<std::chrono::high_resolution_clock>&
        timeout) {
  return Add(
      Item(async_request_state, transfer_destination, transfer, timeout));
}

bool UsbTransfersParametersStorage::ContainsWithAsyncRequestState(
    const TransferAsyncRequestState* async_request_state) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return FindByAsyncRequestState(async_request_state, nullptr);
}

bool UsbTransfersParametersStorage::ContainsAsyncWithDestination(
    const UsbTransferDestination& transfer_destination) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return FindAsyncByDestination(transfer_destination, nullptr);
}

bool UsbTransfersParametersStorage::ContainsAsyncWithLibusbTransfer(
    const libusb_transfer* transfer) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return FindAsyncByLibusbTransfer(transfer, nullptr);
}

UsbTransfersParametersStorage::Item
UsbTransfersParametersStorage::GetByAsyncRequestState(
    const TransferAsyncRequestState* async_request_state) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  Item result;
  GOOGLE_SMART_CARD_CHECK(
      FindByAsyncRequestState(async_request_state, &result));
  return result;
}

UsbTransfersParametersStorage::Item
UsbTransfersParametersStorage::GetAsyncByDestination(
    const UsbTransferDestination& transfer_destination) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  Item result;
  GOOGLE_SMART_CARD_CHECK(
      FindAsyncByDestination(transfer_destination, &result));
  return result;
}

UsbTransfersParametersStorage::Item
UsbTransfersParametersStorage::GetAsyncByLibusbTransfer(
    const libusb_transfer* transfer) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  Item result;
  GOOGLE_SMART_CARD_CHECK(FindAsyncByLibusbTransfer(transfer, &result));
  return result;
}

UsbTransfersParametersStorage::Item
UsbTransfersParametersStorage::GetWithMinTimeout() const {
  const std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(!timeout_mapping_.empty());
  const std::set<const TransferAsyncRequestState*>& state_set =
      timeout_mapping_.begin()->second;

  GOOGLE_SMART_CARD_CHECK(!state_set.empty());
  const TransferAsyncRequestState* const state = *state_set.begin();

  Item result;
  GOOGLE_SMART_CARD_CHECK(FindByAsyncRequestState(state, &result));
  return result;
}

void UsbTransfersParametersStorage::Remove(const Item& item) {
  const std::unique_lock<std::mutex> lock(mutex_);

  // Remove from `async_request_state_mapping_`.
  GOOGLE_SMART_CARD_CHECK(
      async_request_state_mapping_.erase(item.async_request_state.get()));

  if (item.transfer) {
    // Remove from `async_libusb_transfer_mapping_`.
    GOOGLE_SMART_CARD_CHECK(
        async_libusb_transfer_mapping_.erase(item.transfer));

    // Remove from `async_destination_mapping_`.
    const auto sync_destination_mapping_iter =
        async_destination_mapping_.find(item.transfer_destination);
    std::set<const TransferAsyncRequestState*>* const
        transfers_by_sync_destination = &sync_destination_mapping_iter->second;
    GOOGLE_SMART_CARD_CHECK(
        transfers_by_sync_destination->erase(item.async_request_state.get()));
    if (transfers_by_sync_destination->empty())
      async_destination_mapping_.erase(sync_destination_mapping_iter);
  }

  // Remove from `timeout_mapping_`.
  const auto timeout_mapping_iter = timeout_mapping_.find(item.timeout);
  std::set<const TransferAsyncRequestState*>* const transfers_by_timeout =
      &timeout_mapping_iter->second;
  GOOGLE_SMART_CARD_CHECK(
      transfers_by_timeout->erase(item.async_request_state.get()));
  if (transfers_by_timeout->empty())
    timeout_mapping_.erase(timeout_mapping_iter);
}

void UsbTransfersParametersStorage::RemoveByAsyncRequestState(
    const TransferAsyncRequestState* async_request_state) {
  Remove(GetByAsyncRequestState(async_request_state));
}

void UsbTransfersParametersStorage::RemoveByLibusbTransfer(
    const libusb_transfer* transfer) {
  Remove(GetAsyncByLibusbTransfer(transfer));
}

bool UsbTransfersParametersStorage::FindByAsyncRequestState(
    const TransferAsyncRequestState* async_request_state,
    Item* result) const {
  const auto iter = async_request_state_mapping_.find(async_request_state);
  if (iter == async_request_state_mapping_.end())
    return false;
  if (result)
    *result = iter->second;
  return true;
}

bool UsbTransfersParametersStorage::FindAsyncByDestination(
    const UsbTransferDestination& transfer_destination,
    Item* result) const {
  const auto iter = async_destination_mapping_.find(transfer_destination);
  if (iter == async_destination_mapping_.end())
    return false;
  const std::set<const TransferAsyncRequestState*>& transfers = iter->second;
  GOOGLE_SMART_CARD_CHECK(!transfers.empty());
  GOOGLE_SMART_CARD_CHECK(FindByAsyncRequestState(*transfers.begin(), result));
  return true;
}

bool UsbTransfersParametersStorage::FindAsyncByLibusbTransfer(
    const libusb_transfer* transfer,
    Item* result) const {
  GOOGLE_SMART_CARD_CHECK(transfer);
  const auto iter = async_libusb_transfer_mapping_.find(transfer);
  if (iter == async_libusb_transfer_mapping_.end())
    return false;
  if (result)
    *result = iter->second;
  return true;
}

}  // namespace google_smart_card
