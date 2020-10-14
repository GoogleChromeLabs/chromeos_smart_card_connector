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

#include <stdint.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <vector>

#include <libusb.h>

#include <google_smart_card_common/requesting/async_request.h>
#include <google_smart_card_common/requesting/request_result.h>

#include "chrome_usb/types.h"
#include "usb_transfer_destination.h"
#include "usb_transfers_parameters_storage.h"

// Definition of the libusb_context type declared in the libusb headers.
//
// The structure instance is intended to be stored in a shared pointer in order
// to allow passing it as a weak_ptr to the submitted chrome.usb transfers,
// because the actual transfer result may return from chrome.usb API after the
// libusb_context was already destroyed by the client (for the reasoning see the
// next paragraph). FIXME(emaxx): Drop this requirement using the WeakPtrFactory
// concept (inspired by the Chromium source code).
//
// The structure also tracks all libusb transfers that were created through it,
// and implements routing of the chrome.usb transfer results to the correct
// libusb transfers. One particular complexity that is solved by this structure
// is implementing the transfers cancellation. As the chrome.usb API provides no
// means of cancelling the sent transfers, the transfer cancellation operation
// is only emulated by this class. The cancellation is only supported for the
// input transfers, not the output transfers. And what happens when the input
// transfer is canceled is that actually the corresponding chrome.usb transfer
// keeps running. In order not to lose chrome.usb transfer results, once it
// finishes, the structure enqueues all such results internally and passes them
// to the future transfers with the same device and the same set of parameters.
struct libusb_context final
    : public std::enable_shared_from_this<libusb_context> {
  using TransferRequestResult = google_smart_card::RequestResult<
      google_smart_card::chrome_usb::TransferResult>;
  using TransferAsyncRequestState = google_smart_card::AsyncRequestState<
      google_smart_card::chrome_usb::TransferResult>;
  using TransferAsyncRequestStatePtr =
      std::shared_ptr<TransferAsyncRequestState>;
  using UsbTransferDestination = google_smart_card::UsbTransferDestination;
  using UsbTransfersParametersStorage =
      google_smart_card::UsbTransfersParametersStorage;

  // Adds information about a new synchronous transfer into internal structures.
  //
  // The async_request_state argument points to the instance that should be
  // used to store the transfer result. The transfer_destination argument
  // contains the set of parameters that represent the transfer destination,
  // which for input transfers allows to receive the suitable results from the
  // previous canceled transfers.
  void AddSyncTransferInFlight(
      TransferAsyncRequestStatePtr async_request_state,
      const UsbTransferDestination& transfer_destination);

  // Adds information about a new asynchronous transfer into internal
  // structures.
  //
  // The async_request_state argument points to the instance that should be
  // used to store the transfer result. The transfer_destination argument
  // contains the set of parameters that uniquely represent the transfer
  // destination, which for input transfers allows to receive the suitable
  // results from the previous canceled transfers.
  void AddAsyncTransferInFlight(
      TransferAsyncRequestStatePtr async_request_state,
      const UsbTransferDestination& transfer_destination,
      libusb_transfer* transfer);

  // Blocks until the specified input synchronous transfer finishes.
  //
  // The transfer_destination argument contains the set of parameters that
  // uniquely represent the transfer destination, which for input transfers
  // allows to receive the suitable results from the previous canceled
  // transfers.
  //
  // It is guaranteed that the instance pointed by the async_request_state
  // argument will contain the transfer result once the method finishes.
  void WaitAndProcessInputSyncTransferReceivedResult(
      TransferAsyncRequestStatePtr async_request_state,
      const UsbTransferDestination& transfer_destination);

  // Blocks until the specified output synchronous transfer finishes.
  //
  // It is guaranteed that the instance pointed by the async_request_state
  // argument will contain the transfer result once the method finishes.
  void WaitAndProcessOutputSyncTransferReceivedResult(
      TransferAsyncRequestStatePtr async_request_state);

  // Blocks until either a new asynchronous transfer result is received (in
  // which case the transfer callback is executed), or the specified completed
  // flag becomes non-zero, or the timeout happens (whatever is detected first).
  //
  // For the general information regarding libusb event handling (and, in
  // particular, the role of the completed argument), refer to
  // <http://libusb.org/static/api-1.0/mtasync.html>.
  void WaitAndProcessAsyncTransferReceivedResults(
      const std::chrono::time_point<std::chrono::high_resolution_clock>&
          timeout_time_point,
      int* completed);

  // Tries to cancel the specified asynchronous transfer.
  //
  // Returns whether the operation finished successfully. Only input transfers
  // may be canceled successfully; the cancellation succeeds only if the
  // transfer was already submitted, but not completed or canceled yet.
  bool CancelTransfer(libusb_transfer* transfer);

  // Adds a result of a finished input chrome.usb transfer.
  //
  // The transfer_destination argument contains the set of parameters that
  // uniquely represent the transfer destination, which allows to deliver the
  // results to the corresponding input transfer in flight.
  void OnInputTransferResultReceived(
      const UsbTransferDestination& transfer_destination,
      TransferRequestResult result);

  // Adds a result of a finished output chrome.usb transfer.
  //
  // The async_request_state argument points to the instance that should be used
  // to store the transfer result.
  void OnOutputTransferResultReceived(
      TransferAsyncRequestStatePtr async_request_state,
      TransferRequestResult result);

 private:
  void AddTransferInFlight(TransferAsyncRequestStatePtr async_request_state,
                           const UsbTransferDestination& transfer_destination,
                           libusb_transfer* transfer);

  void RemoveTransferInFlight(
      const TransferAsyncRequestState* async_request_state);

  bool ExtractAsyncTransferStateUpdate(
      TransferAsyncRequestStatePtr* async_request_state,
      TransferRequestResult* result);
  bool ExtractAsyncTransferStateCancellationUpdate(
      TransferAsyncRequestStatePtr* async_request_state,
      TransferRequestResult* result);
  bool ExtractOutputAsyncTransferStateUpdate(
      TransferAsyncRequestStatePtr* async_request_state,
      TransferRequestResult* result);
  bool ExtractInputAsyncTransferStateUpdate(
      TransferAsyncRequestStatePtr* async_request_state,
      TransferRequestResult* result);

  bool ExtractMatchingInputTransferResult(
      const UsbTransferDestination& transfer_destination,
      TransferRequestResult* result);

  void SetTransferResult(TransferAsyncRequestState* async_request_state,
                         TransferRequestResult result);

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  // This member stores parameters of all transfers that are currently in
  // flight.
  UsbTransfersParametersStorage transfers_in_flight_;
  // This mapping stores the received data for the finished input transfer
  // requests.
  //
  // The mapping groups its items according to the transfer destination, which
  // allows to route the result to the corresponding transfer (this is important
  // when the original transfer was previously canceled).
  //
  // Each transfers group is stored in a queue, which allows to preserve the
  // relative order in which they where received.
  std::map<UsbTransferDestination, std::queue<TransferRequestResult>>
      received_input_transfer_result_map_;
  // This member stores the received data for the finished output transfer
  // requests.
  std::map<TransferAsyncRequestStatePtr, TransferRequestResult>
      received_output_transfer_result_map_;
  // This set stores pointers to the transfers for which the cancellation was
  // requests.
  std::set<libusb_transfer*> transfers_to_cancel_;
};

// Definition of the libusb_device type declared in the libusb headers.
//
// The structure corresponds to the Device structure in chrome.usb interface.
struct libusb_device final {
  // Creates a new structure with the reference counter equal to 1.
  libusb_device(libusb_context* context,
                const google_smart_card::chrome_usb::Device& chrome_usb_device);

  ~libusb_device();

  libusb_context* context() const;
  const google_smart_card::chrome_usb::Device& chrome_usb_device() const;

  // Increments the reference counter.
  void AddReference();
  // Decrements the reference counter. If it becomes equal to zero, then deletes
  // the self instance.
  void RemoveReference();

 private:
  libusb_context* context_;
  google_smart_card::chrome_usb::Device chrome_usb_device_;
  std::atomic_int reference_count_{1};
};

// Definition of the libusb_device_handle type declared in the libusb headers.
//
// The structure corresponds to the ConnectionHandle structure in chrome.usb
// interface.
struct libusb_device_handle final {
  // Constructs the structure and increments the reference counter of the
  // specified libusb_device instance.
  libusb_device_handle(libusb_device* device,
                       const google_smart_card::chrome_usb::ConnectionHandle&
                           chrome_usb_connection_handle);

  // Destructs the structure and decrements the reference counter of the
  // specified libusb_device instance.
  ~libusb_device_handle();

  libusb_device* device() const;
  libusb_context* context() const;
  const google_smart_card::chrome_usb::ConnectionHandle&
  chrome_usb_connection_handle() const;

 private:
  libusb_device* device_;
  google_smart_card::chrome_usb::ConnectionHandle chrome_usb_connection_handle_;
};

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_OPAQUE_TYPES_H_
