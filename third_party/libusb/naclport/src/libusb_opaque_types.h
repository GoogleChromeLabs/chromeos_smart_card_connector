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

// Definitions of the libusb opaque types declared in the libusb.h header
// (consumers should operate only with pointers to these structures).
//
// Note that the Style Guide is violated here, as having complex methods and
// private fields in structs is disallowed - but it's not possible to switch to
// classes as libusb headers already declared them as structs.

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_OPAQUE_TYPES_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_OPAQUE_TYPES_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>

#include <google_smart_card_common/requesting/async_request.h>
#include <google_smart_card_common/requesting/request_result.h>

#include "libusb.h"

#include "chrome_usb/api_bridge.h"
#include "chrome_usb/types.h"

struct libusb_context {

  void AddAsyncTransfer(
      libusb_transfer* transfer,
      google_smart_card::AsyncRequest** async_request);

  void RemoveAsyncTransfer(libusb_transfer* transfer);

  bool CancelAsyncTransfer(libusb_transfer* transfer);

  void AddCompletedAsyncTransfer(
      libusb_transfer* transfer,
      google_smart_card::RequestResult<
          google_smart_card::chrome_usb::TransferResult> request_result);

  bool WaitAndExtractCompletedAsyncTransfer(
      int timeout_seconds,
      libusb_transfer** transfer,
      google_smart_card::RequestResult<
          google_smart_card::chrome_usb::TransferResult>* request_result);

 private:
  struct CompletedAsyncTransfer {
    libusb_transfer* transfer;
    google_smart_card::RequestResult<
        google_smart_card::chrome_usb::TransferResult> request_result;

    CompletedAsyncTransfer(
        libusb_transfer* transfer,
        google_smart_card::RequestResult<
            google_smart_card::chrome_usb::TransferResult> request_result);
  };

  google_smart_card::AsyncRequest* GetAsyncTransferRequest(
      libusb_transfer* transfer);

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::unordered_map<libusb_transfer*, google_smart_card::AsyncRequest>
  async_transfer_request_map_;
  std::queue<CompletedAsyncTransfer> completed_async_transfers_;
};

struct libusb_device {
  libusb_device(
      libusb_context* context,
      const google_smart_card::chrome_usb::Device& chrome_usb_device);

  ~libusb_device();

  libusb_context* context();
  google_smart_card::chrome_usb::Device& chrome_usb_device();

  void AddReference();
  void RemoveReference();

 private:
  libusb_context* context_;
  google_smart_card::chrome_usb::Device chrome_usb_device_;
  std::atomic_int reference_count_;
};

struct libusb_device_handle {
  libusb_device_handle(
      libusb_device* device,
      const google_smart_card::chrome_usb::ConnectionHandle&
          chrome_usb_connection_handle);

  ~libusb_device_handle();

  libusb_device* device;
  google_smart_card::chrome_usb::ConnectionHandle chrome_usb_connection_handle;
};

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_OPAQUE_TYPES_H_
