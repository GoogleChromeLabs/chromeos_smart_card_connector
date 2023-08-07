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

#include <chrono>
#include <utility>

#include "common/cpp/src/google_smart_card_common/logging/logging.h"

void libusb_context::AddAsyncTransferInFlight(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination,
    libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);
  GOOGLE_SMART_CARD_CHECK(transfer);

  const std::unique_lock<std::mutex> lock(mutex_);

  AddTransferInFlight(async_request_state, transfer_destination, transfer);
}

void libusb_context::WaitAndProcessAsyncTransferReceivedResults(
    const std::chrono::time_point<std::chrono::high_resolution_clock>&
        timeout_time_point,
    int* completed) {
  TransferAsyncRequestStatePtr async_request_state;
  TransferRequestResult result;

  {
    std::unique_lock<std::mutex> lock(mutex_);

    // Start the event loop.
    for (;;) {
      if (completed && *completed) {
        // The transfer has already been completed (either previously or in some
        // parallel thread running the same function).
        return;
      }

      if (ExtractAsyncTransferStateUpdate(&async_request_state, &result)) {
        // Picked up a transfer that can be populated with a result. Stop
        // tracking the transfer and exit the event loop (to populate the
        // transfer with the result outside the mutex - see the comment below).
        RemoveTransferInFlight(async_request_state.get());
        break;
      }

      const std::chrono::time_point<std::chrono::high_resolution_clock>
          min_transfer_timeout = GetMinTransferTimeout();

      // Wait until a transfer result arrives, or some transfer times out, or we
      // time out according to `timeout_time_point`, or the conditonal variable
      // wakes up spuriously.
      const auto wait_until_time_point =
          std::min(timeout_time_point, min_transfer_timeout);
      condition_.wait_until(lock, wait_until_time_point);

      // Immediately exit if we timed out according to `timeout_time_point`.
      if (std::chrono::high_resolution_clock::now() >= timeout_time_point)
        return;
    }
  }

  GOOGLE_SMART_CARD_CHECK(async_request_state);
  // TODO(#429): Assert the result is non-empty.
  SetTransferResult(async_request_state.get(), std::move(result));

  {
    // In case some other thread is waiting for this particular transfer's
    // result via the transfer's completed flag, let it awake. Note that it's
    // crucial to do this under mutex again, since otherwise the other thread
    // might miss the notification.
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.notify_all();
  }
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

  received_input_transfer_result_map_[transfer_destination].push(
      std::move(result));

  condition_.notify_all();
}

void libusb_context::OnOutputTransferResultReceived(
    TransferAsyncRequestStatePtr async_request_state,
    TransferRequestResult result) {
  const std::unique_lock<std::mutex> lock(mutex_);

  if (!transfers_in_flight_.ContainsWithAsyncRequestState(
          async_request_state.get())) {
    // The output transfer timed out in the meantime, so just discard the
    // result.
    // Note that the transfer couldn't have been cancelled, as
    // `CancelTransfer()` only allows input transfers.
    return;
  }

  GOOGLE_SMART_CARD_CHECK(received_output_transfer_result_map_
                              .emplace(async_request_state, std::move(result))
                              .second);

  condition_.notify_all();
}

void libusb_context::AddTransferInFlight(
    TransferAsyncRequestStatePtr async_request_state,
    const UsbTransferDestination& transfer_destination,
    libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);

  std::chrono::time_point<std::chrono::high_resolution_clock> timeout;
  if (transfer->timeout == 0) {
    // A zero timeout field denotes an infinite timeout.
    timeout =
        std::chrono::time_point<std::chrono::high_resolution_clock>::max();
  } else {
    timeout = std::chrono::high_resolution_clock::now() +
              std::chrono::milliseconds(transfer->timeout);
  }

  transfers_in_flight_.Add(async_request_state, transfer_destination, transfer,
                           timeout);

  condition_.notify_all();
}

void libusb_context::RemoveTransferInFlight(
    const TransferAsyncRequestState* async_request_state) {
  GOOGLE_SMART_CARD_CHECK(async_request_state);

  const UsbTransfersParametersStorage::Item parameters =
      transfers_in_flight_.GetByAsyncRequestState(async_request_state);
  libusb_transfer* const transfer = parameters.transfer;
  const TransferAsyncRequestStatePtr async_request_state_ptr =
      parameters.async_request_state;

  transfers_in_flight_.RemoveByAsyncRequestState(async_request_state);

  // Note that the entry can be present in that map, for example, when the
  // result arrived shortly before the transfer timed out.
  received_output_transfer_result_map_.erase(async_request_state_ptr);

  if (transfer) {
    // Note that this assertion relies on the fact that transfer cancellation
    // has precedence over all other events (i.e., the results processing and
    // the timeout processing - see `GetAsyncTransferStateUpdate()`).
    GOOGLE_SMART_CARD_CHECK(!transfers_to_cancel_.count(transfer));
  }
}

std::chrono::time_point<std::chrono::high_resolution_clock>
libusb_context::GetMinTransferTimeout() const {
  if (transfers_in_flight_.empty())
    return std::chrono::time_point<std::chrono::high_resolution_clock>::max();
  return transfers_in_flight_.GetWithMinTimeout().timeout;
}

bool libusb_context::ExtractAsyncTransferStateUpdate(
    TransferAsyncRequestStatePtr* async_request_state,
    TransferRequestResult* result) {
  // Note that it's crucial to do this check of canceled requests before all
  // other options, because the cancellation of the transfer, after it got
  // accepted, should have precedence over receiving of results for the transfer
  // (and this is asserted in `RemoveTransferInFlight()`).
  return ExtractAsyncTransferStateCancellationUpdate(async_request_state,
                                                     result) ||
         ExtractTimedOutTransfer(async_request_state, result) ||
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

  *async_request_state = transfers_in_flight_.GetAsyncByLibusbTransfer(transfer)
                             .async_request_state;
  *result = TransferRequestResult::CreateCanceled();
  return true;
}

bool libusb_context::ExtractTimedOutTransfer(
    TransferAsyncRequestStatePtr* async_request_state,
    TransferRequestResult* result) {
  if (transfers_in_flight_.empty())
    return false;
  UsbTransfersParametersStorage::Item nearest =
      transfers_in_flight_.GetWithMinTimeout();
  if (std::chrono::high_resolution_clock::now() < nearest.timeout)
    return false;
  *async_request_state = nearest.async_request_state;
  // TODO(#47): Use a common constant here that can be checked in
  // `LibusbJsProxy`, so that it can distinguish timeouts from other failures.
  *result = TransferRequestResult::CreateFailed("Timed out");
  return true;
}

bool libusb_context::ExtractOutputAsyncTransferStateUpdate(
    TransferAsyncRequestStatePtr* async_request_state,
    TransferRequestResult* result) {
  for (auto& results_map_item : received_output_transfer_result_map_) {
    const TransferAsyncRequestStatePtr& stored_async_request =
        results_map_item.first;

    if (!transfers_in_flight_.GetByAsyncRequestState(stored_async_request.get())
             .transfer) {
      // Skip this transfer as it's a synchronous one.
      continue;
    }

    *async_request_state = stored_async_request;
    *result = std::move(results_map_item.second);
    GOOGLE_SMART_CARD_CHECK(
        received_output_transfer_result_map_.erase(*async_request_state));
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
      *async_request_state =
          transfers_in_flight_.GetAsyncByDestination(transfer_destination)
              .async_request_state;

      *result = std::move(stored_results->front());
      stored_results->pop();
      if (stored_results->empty()) {
        GOOGLE_SMART_CARD_CHECK(
            received_input_transfer_result_map_.erase(transfer_destination));
      }

      return true;
    }
  }

  return false;
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

libusb_device::libusb_device(libusb_context* context,
                             const google_smart_card::LibusbJsDevice& js_device)
    : context_(context), js_device_(js_device) {
  GOOGLE_SMART_CARD_CHECK(context);
}

libusb_device::~libusb_device() {
  GOOGLE_SMART_CARD_CHECK(!reference_count_);
}

libusb_context* libusb_device::context() const {
  return context_;
}

const google_smart_card::LibusbJsDevice& libusb_device::js_device() const {
  return js_device_;
}

google_smart_card::optional<google_smart_card::LibusbJsConfigurationDescriptor>
libusb_device::js_config() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return js_config_;
}

void libusb_device::set_js_config(
    google_smart_card::optional<
        google_smart_card::LibusbJsConfigurationDescriptor> new_js_config) {
  std::unique_lock<std::mutex> lock(mutex_);
  js_config_ = std::move(new_js_config);
}

void libusb_device::AddReference() {
  std::unique_lock<std::mutex> lock(mutex_);
  ++reference_count_;
  GOOGLE_SMART_CARD_CHECK(reference_count_ > 1);
}

void libusb_device::RemoveReference() {
  int new_reference_count;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    new_reference_count = --reference_count_;
  }
  GOOGLE_SMART_CARD_CHECK(new_reference_count >= 0);
  if (!new_reference_count)
    delete this;
}

libusb_device_handle::libusb_device_handle(libusb_device* device,
                                           int64_t js_device_handle)
    : device_(device), js_device_handle_(js_device_handle) {
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

int64_t libusb_device_handle::js_device_handle() const {
  return js_device_handle_;
}
