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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFERS_PARAMETERS_STORAGE_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFERS_PARAMETERS_STORAGE_H_

#include <map>
#include <memory>
#include <mutex>
#include <set>

#include <libusb.h>

#include <google_smart_card_common/requesting/async_request.h>

#include "chrome_usb/types.h"
#include "usb_transfer_destination.h"

namespace google_smart_card {

// This class provides a sort of multi-index map between various USB transfer
// structures.
//
// One index has the TransferAsyncRequestStatePtr type, which corresponds to the
// transfer result storage of each synchronous and asynchronous transfer that is
// executed by the LibusbOverChromeUsb class methods.
//
// Another index has the UsbTransferDestination type, which uniquely represents
// the target USB device and a set of transfer parameters.
//
// The third index is a pointer to libusb_transfer structure. This index is
// available only for asynchronous transfers.
class UsbTransfersParametersStorage final {
 public:
  using TransferAsyncRequestState =
      AsyncRequestState<chrome_usb::TransferResult>;
  using TransferAsyncRequestStatePtr =
      std::shared_ptr<TransferAsyncRequestState>;

  UsbTransfersParametersStorage();
  UsbTransfersParametersStorage(const UsbTransfersParametersStorage&) = delete;
  UsbTransfersParametersStorage& operator=(
      const UsbTransfersParametersStorage&) = delete;
  ~UsbTransfersParametersStorage();

  struct Item {
    Item();

    Item(TransferAsyncRequestStatePtr async_request_state,
         const UsbTransferDestination& transfer_destination,
         libusb_transfer* transfer);

    TransferAsyncRequestStatePtr async_request_state;
    UsbTransferDestination transfer_destination;
    libusb_transfer* transfer;
  };

  void Add(const Item& item);

  void Add(TransferAsyncRequestStatePtr async_request_state,
           const UsbTransferDestination& transfer_destination,
           libusb_transfer* transfer);

  bool ContainsWithAsyncRequestState(
      const TransferAsyncRequestState* async_request_state) const;

  bool ContainsAsyncWithDestination(
      const UsbTransferDestination& transfer_destination) const;

  bool ContainsAsyncWithLibusbTransfer(const libusb_transfer* transfer) const;

  Item GetByAsyncRequestState(
      const TransferAsyncRequestState* async_request_state) const;

  Item GetAsyncByDestination(
      const UsbTransferDestination& transfer_destination) const;

  Item GetAsyncByLibusbTransfer(const libusb_transfer* transfer) const;

  void Remove(const Item& item);

  void RemoveByAsyncRequestState(
      const TransferAsyncRequestState* async_request_state);

  void RemoveByLibusbTransfer(const libusb_transfer* transfer);

 private:
  bool FindByAsyncRequestState(
      const TransferAsyncRequestState* async_request_state,
      Item* result) const;
  bool FindAsyncByDestination(
      const UsbTransferDestination& transfer_destination,
      Item* result) const;
  bool FindAsyncByLibusbTransfer(const libusb_transfer* transfer,
                                 Item* result) const;

  mutable std::mutex mutex_;
  std::map<const TransferAsyncRequestState*, Item> async_request_state_mapping_;
  std::map<UsbTransferDestination, std::set<const TransferAsyncRequestState*>>
      async_destination_mapping_;
  std::map<const libusb_transfer*, Item> async_libusb_transfer_mapping_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFERS_PARAMETERS_STORAGE_H_
