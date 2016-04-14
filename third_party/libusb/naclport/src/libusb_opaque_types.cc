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

#include <google_smart_card_common/logging/logging.h>

void libusb_context::AddAsyncTransfer(
    libusb_transfer* transfer,
    google_smart_card::AsyncRequest** async_request) {
  GOOGLE_SMART_CARD_CHECK(transfer);
  GOOGLE_SMART_CARD_CHECK(async_request);

  const std::unique_lock<std::mutex> lock(mutex_);

  // The same libusb_transfer object can be used by the consumer multiple
  // times without free'ing it, so it's essential to remove the previously
  // existing AsyncRequest, if any.
  async_transfer_request_map_.erase(transfer);

  const auto request_iter = async_transfer_request_map_.emplace(
      transfer, google_smart_card::AsyncRequest()).first;
  *async_request = &request_iter->second;
}

void libusb_context::RemoveAsyncTransfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  const std::unique_lock<std::mutex> lock(mutex_);

  async_transfer_request_map_.erase(transfer);
}

bool libusb_context::CancelAsyncTransfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  google_smart_card::AsyncRequest* async_request = GetAsyncTransferRequest(
      transfer);
  if (!async_request)
    return false;

  // Note that it's essential to perform the cancellation outside the mutex,
  // as the cancellation may result in enqueuing the transfer's result, that
  // will lock the mutex itself.
  //
  // Note: The data race may not occur here, as the correct consumer should
  // not free the transfer at the same time as cancelling it.
  return async_request->Cancel();
}

void libusb_context::AddCompletedAsyncTransfer(
    libusb_transfer* transfer,
    google_smart_card::RequestResult<
        google_smart_card::chrome_usb::TransferResult> request_result) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  const std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(async_transfer_request_map_.count(transfer));

  completed_async_transfers_.emplace(transfer, std::move(request_result));
  condition_.notify_all();
}

bool libusb_context::WaitAndExtractCompletedAsyncTransfer(
    int timeout_seconds,
    libusb_transfer** transfer,
    google_smart_card::RequestResult<
        google_smart_card::chrome_usb::TransferResult>* request_result) {
  GOOGLE_SMART_CARD_CHECK(transfer);
  GOOGLE_SMART_CARD_CHECK(request_result);

  std::unique_lock<std::mutex> lock(mutex_);

  if (!condition_.wait_for(
           lock,
           std::chrono::seconds(timeout_seconds),
           [this] {
             return !completed_async_transfers_.empty();
           })) {
    return false;
  }

  *transfer = completed_async_transfers_.front().transfer;
  *request_result = std::move(
      completed_async_transfers_.front().request_result);
  completed_async_transfers_.pop();

  return true;
}

libusb_context::CompletedAsyncTransfer::CompletedAsyncTransfer(
    libusb_transfer* transfer,
    google_smart_card::RequestResult<
        google_smart_card::chrome_usb::TransferResult> request_result)
    : transfer(transfer),
      request_result(std::move(request_result)) {}

google_smart_card::AsyncRequest* libusb_context::GetAsyncTransferRequest(
    libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_CHECK(transfer);

  const std::unique_lock<std::mutex> lock(mutex_);

  const auto request_iter = async_transfer_request_map_.find(transfer);
  if (request_iter == async_transfer_request_map_.end())
    return nullptr;
  return &request_iter->second;
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

libusb_context* libusb_device::context() {
  return context_;
}

google_smart_card::chrome_usb::Device& libusb_device::chrome_usb_device() {
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
    : device(device),
      chrome_usb_connection_handle(chrome_usb_connection_handle) {
  GOOGLE_SMART_CARD_CHECK(device);
  device->AddReference();
}

libusb_device_handle::~libusb_device_handle() {
  device->RemoveReference();
}
