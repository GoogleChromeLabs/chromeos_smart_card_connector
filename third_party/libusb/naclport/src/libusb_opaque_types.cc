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

#include "libusb_opaque_types.h"

#include <utility>

#include <google_smart_card_common/logging/logging.h>

void libusb_context::AddSyncTransferInFlight(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);

  const std::unique_lock<std::mutex> lock(mutex_);

  AddTransferInFlight(async_request_state, transfer_destination, nullptr);
}

void libusb_context::AddAsyncTransferInFlight(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination,
    libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);
  GOOGLE_SMART_CARD_CHECK(transfer);

  const std::unique_lock<std::mutex> lock(mutex_);

  AddTransferInFlight(async_request_state, transfer_destination, transfer);
}

void libusb_context::WaitAndProcessInputSyncTransferReceivedResult(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination) {
  TransferRequestResult result;

  {
    std::unique_lock<std::mutex> lock(mutex_);

    for (;;) {
      GOOGLE_SMART_CARD_CHECK(
          transfers_in_flight_.ContainsWithAsyncRequestState(
              async_request_state.get()));

      if (ExtractMatchingInputTransferResult(transfer_destination, &result)) {
        RemoveTransferInFlight(async_request_state.get());
        break;
      }

      condition_.wait(lock);
    }
  }

  SetTransferResult(async_request_state.get(), std::move(result));
}

void libusb_context::WaitAndProcessOutputSyncTransferReceivedResult(
    TransferAsyncRequestStatePtr async_request_state) {
  TransferRequestResult result;

  {
    std::unique_lock<std::mutex> lock(mutex_);

    for (;;) {
      GOOGLE_SMART_CARD_CHECK(
          transfers_in_flight_.ContainsWithAsyncRequestState(
              async_request_state.get()));

      if (received_output_transfer_result_map_.count(async_request_state)) {
        result = std::move(
            received_output_transfer_result_map_[async_request_state]);
        received_output_transfer_result_map_.erase(async_request_state);
        RemoveTransferInFlight(async_request_state.get());
        break;
      }

      condition_.wait(lock);
    }
  }

  SetTransferResult(async_request_state.get(), std::move(result));
}

void libusb_context::WaitAndProcessAsyncTransferReceivedResults(
    const std::chrono::time_point<std::chrono::high_resolution_clock>&
    timeout_time_point,
    int* completed) {
  TransferAsyncRequestStatePtr async_request_state;
  TransferRequestResult result;

  {
    std::unique_lock<std::mutex> lock(mutex_);

    for (;;) {
      if (completed && *completed) {
        // The transfer has already been completed (either previously or in some
        // parallel thread running the same function).
        return;
      }

      if (ExtractAsyncTransferStateUpdate(&async_request_state, &result)) {
        RemoveTransferInFlight(async_request_state.get());
        break;
      }

      if (condition_.wait_until(lock, timeout_time_point) ==
          std::cv_status::timeout) {
        return;
      }
    }
  }

  SetTransferResult(async_request_state.get(), std::move(result));
}

bool libusb_context::CancelTransfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  const std::unique_lock<std::mutex> lock(mutex_);

  if (!transfers_in_flight_.ContainsAsyncWithLibusbTransfer(transfer)) {
    // The transfer is either already completed (which includes cases where its
    // callback is just about to be called) or is not sent yet (which could
    // happen with a bad consumer code).
    return false;
  }

  const UsbTransfersParametersStorage::Item parameters =
      transfers_in_flight_.GetAsyncByLibusbTransfer(transfer);

  if (!parameters.transfer_destination.IsInputDirection()) {
    // Cancellation of output transfers is not supported.
    return false;
  }

  if (transfers_to_cancel_.count(transfer)) {
    // Cancellation of this transfer was already requested previously.
    return false;
  }

  transfers_to_cancel_.insert(transfer);

  condition_.notify_all();

  return true;
}

void libusb_context::OnInputTransferResultReceived(
    const UsbTransferDestination& transfer_destination,
    TransferRequestResult result) {
  const std::unique_lock<std::mutex> lock(mutex_);

  received_input_transfer_result_map_[transfer_destination].push(result);

  condition_.notify_all();
}

void libusb_context::OnOutputTransferResultReceived(
    TransferAsyncRequestStatePtr async_request_state,
    TransferRequestResult result) {
  const std::unique_lock<std::mutex> lock(mutex_);

  // Note that the check is correct because cancellation of output transfers
  // never happens (this is guaranteed by the implementation of the
  // CancelTransfer method).
  GOOGLE_SMART_CARD_CHECK(transfers_in_flight_.ContainsWithAsyncRequestState(
      async_request_state.get()));

  GOOGLE_SMART_CARD_CHECK(received_output_transfer_result_map_.emplace(
      async_request_state, result).second);

  condition_.notify_all();
}

void libusb_context::AddTransferInFlight(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination,
    libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);

  transfers_in_flight_.Add(async_request_state, transfer_destination, transfer);

  condition_.notify_all();
}

void libusb_context::RemoveTransferInFlight(
    const TransferAsyncRequestState* async_request_state) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);

  const UsbTransfersParametersStorage::Item parameters =
      transfers_in_flight_.GetByAsyncRequestState(async_request_state);
  const UsbTransferDestination& transfer_destination =
      parameters.transfer_destination;
  libusb_transfer* const transfer = parameters.transfer;
  const TransferAsyncRequestStatePtr async_request_state_ptr =
      parameters.async_request_state;

  transfers_in_flight_.RemoveByAsyncRequestState(async_request_state);

  // Note that the check is correct because cancellation of output transfers
  // never happens (this is guaranteed by the implementation of the
  // CancelTransfer method).
  GOOGLE_SMART_CARD_CHECK(!received_output_transfer_result_map_.count(
      async_request_state_ptr));

  if (transfer) {
    // Note that this assertion relies on the fact that transfer cancellation
    // has precedence over receiving of results for the transfer (this is
    // guaranteed by the implementation of the GetAsyncTransferStateUpdate
    // method).
    GOOGLE_SMART_CARD_CHECK(!transfers_to_cancel_.count(transfer));
  }
}

bool libusb_context::ExtractAsyncTransferStateUpdate(
    TransferAsyncRequestStatePtr* async_request_state,
    TransferRequestResult* result) {
  // Note that it's crucial to do this check of canceled requests before all
  // other options, because the cancellation of the transfer, if accepted,
  // should have precedence over receiving of results for the transfer (and this
  // is asserted in method RemoveTransferInFlight).
  return
      ExtractAsyncTransferStateCancellationUpdate(async_request_state, result) ||
      ExtractOutputAsyncTransferStateUpdate(async_request_state, result) ||
      ExtractInputAsyncTransferStateUpdate(async_request_state, result);
}

bool libusb_context::ExtractAsyncTransferStateCancellationUpdate(
    TransferAsyncRequestStatePtr* async_request_state,
    TransferRequestResult* result) {
  if (transfers_to_cancel_.empty())
    return false;

  const auto iter = transfers_to_cancel_.begin();
  libusb_transfer* const transfer = *iter;
  transfers_to_cancel_.erase(iter);

  *async_request_state = transfers_in_flight_.GetAsyncByLibusbTransfer(
      transfer).async_request_state;
  *result = TransferRequestResult::CreateCanceled();
  return true;
}

bool libusb_context::ExtractOutputAsyncTransferStateUpdate(
    TransferAsyncRequestStatePtr* async_request_state,
    TransferRequestResult* result) {
  for (auto& results_map_item : received_output_transfer_result_map_) {
    const TransferAsyncRequestStatePtr& stored_async_request =
        results_map_item.first;

    if (!transfers_in_flight_.GetByAsyncRequestState(
             stored_async_request.get()).transfer) {
      // Skip this transfer as it's a synchronous one.
      continue;
    }

    *async_request_state = stored_async_request;
    *result = std::move(results_map_item.second);
    GOOGLE_SMART_CARD_CHECK(received_output_transfer_result_map_.erase(
        *async_request_state));
    return true;
  }

  return false;
}

bool libusb_context::ExtractInputAsyncTransferStateUpdate(
    TransferAsyncRequestStatePtr* async_request_state,
    TransferRequestResult* result) {
  for (auto& results_map_item : received_input_transfer_result_map_) {
    const UsbTransferDestination transfer_destination = results_map_item.first;
    std::queue<TransferRequestResult>* const stored_results =
        &results_map_item.second;

    if (transfers_in_flight_.ContainsAsyncWithDestination(
            transfer_destination)) {
      *async_request_state = transfers_in_flight_.GetAsyncByDestination(
          transfer_destination).async_request_state;

      *result = stored_results->front();
      stored_results->pop();
      if (stored_results->empty()) {
        GOOGLE_SMART_CARD_CHECK(received_input_transfer_result_map_.erase(
            transfer_destination));
      }

      return true;
    }
  }

  return false;
}

bool libusb_context::ExtractMatchingInputTransferResult(
    const UsbTransferDestination& transfer_destination,
    TransferRequestResult* result) {
  const auto iter = received_input_transfer_result_map_.find(
      transfer_destination);
  if (iter == received_input_transfer_result_map_.end())
    return false;
  std::queue<TransferRequestResult>* results_queue = &iter->second;

  GOOGLE_SMART_CARD_CHECK(!results_queue->empty());
  *result = results_queue->front();
  results_queue->pop();

  if (results_queue->empty())
    received_input_transfer_result_map_.erase(iter);

  return true;
}

void libusb_context::SetTransferResult(
    TransferAsyncRequestState* async_request_state,
    TransferRequestResult result) {
  // Note that this code must be executed outside the mutex lock of the same
  // thread, as execution of the customer-provided transfer callback may try to
  // operate with the same libusb_context.
  //
  // However, STL provides no way to implement an assertion on this.

  // Note that the check is correct, as all references to this transfer in the
  // internal structures should have been removed when extracting the transfer
  // with the corresponding result, and no other thread will try to assign a
  // result to this transfer.
  GOOGLE_SMART_CARD_CHECK(async_request_state->SetResult(std::move(result)));

  condition_.notify_all();
}

libusb_device::libusb_device(
    libusb_context* context,
    const google_smart_card::chrome_usb::Device& chrome_usb_device)
    : context_(context),
      chrome_usb_device_(chrome_usb_device),
      reference_count_(1) {
  GOOGLE_SMART_CARD_CHECK(context);
}

libusb_device::~libusb_device() {
  GOOGLE_SMART_CARD_CHECK(!reference_count_);
}

libusb_context* libusb_device::context() const {
  return context_;
}

const google_smart_card::chrome_usb::Device&
libusb_device::chrome_usb_device() const {
  return chrome_usb_device_;
}

void libusb_device::AddReference() {
  const int new_reference_count = ++reference_count_;
  GOOGLE_SMART_CARD_CHECK(new_reference_count > 1);
}

void libusb_device::RemoveReference() {
  const int new_reference_count = --reference_count_;
  GOOGLE_SMART_CARD_CHECK(new_reference_count >= 0);
  if (!new_reference_count)
    delete this;
}

libusb_device_handle::libusb_device_handle(
    libusb_device* device,
    const google_smart_card::chrome_usb::ConnectionHandle&
        chrome_usb_connection_handle)
    : device_(device),
      chrome_usb_connection_handle_(chrome_usb_connection_handle) {
  GOOGLE_SMART_CARD_CHECK(device_);
  device_->AddReference();
}

libusb_device_handle::~libusb_device_handle() {
  device_->RemoveReference();
}

libusb_device* libusb_device_handle::device() const {
  return device_;
}

libusb_context* libusb_device_handle::context() const {
  return device_->context();
}

const google_smart_card::chrome_usb::ConnectionHandle&
libusb_device_handle::chrome_usb_connection_handle() const {
  return chrome_usb_connection_handle_;
}
