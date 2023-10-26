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

#include <deque>
#include <map>
#include <memory>
#include <mutex>

#include <libusb.h>

#include "common/cpp/src/public/optional.h"
#include "common/cpp/src/public/requesting/async_request.h"
#include "common/cpp/src/public/requesting/remote_call_async_request.h"
#include "third_party/libusb/webport/src/libusb_js_proxy_data_model.h"
#include "third_party/libusb/webport/src/usb_transfer_destination.h"

namespace google_smart_card {

// This class provides a sort of multi-index map between various USB transfer
// structures.
//
// One index has the TransferAsyncRequestStatePtr type, which corresponds to the
// transfer result storage of each synchronous and asynchronous transfer that is
// executed by the LibusbJsProxy class methods.
//
// Another index has the UsbTransferDestination type, which uniquely represents
// the target USB device and a set of transfer parameters.
//
// The third index is a pointer to libusb_transfer structure. This index is
// available only for asynchronous transfers.
class UsbTransfersParametersStorage final {
 public:
  using TransferAsyncRequestState = AsyncRequestState<LibusbJsTransferResult>;
  using TransferAsyncRequestStatePtr =
      std::shared_ptr<TransferAsyncRequestState>;

  UsbTransfersParametersStorage();
  UsbTransfersParametersStorage(const UsbTransfersParametersStorage&) = delete;
  UsbTransfersParametersStorage& operator=(
      const UsbTransfersParametersStorage&) = delete;
  ~UsbTransfersParametersStorage();

  struct Info {
    TransferAsyncRequestStatePtr async_request_state;
    UsbTransferDestination transfer_destination;
    libusb_transfer* transfer = nullptr;
    std::chrono::time_point<std::chrono::high_resolution_clock> timeout;
  };

  bool empty() const;

  void Add(TransferAsyncRequestStatePtr async_request_state,
           const UsbTransferDestination& transfer_destination,
           libusb_transfer* transfer,
           RemoteCallAsyncRequest prepared_js_call,
           const std::chrono::time_point<std::chrono::high_resolution_clock>&
               timeout);

  bool ContainsWithAsyncRequestState(
      const TransferAsyncRequestState* async_request_state) const;
  bool ContainsAsyncWithDestination(
      const UsbTransferDestination& transfer_destination) const;
  bool ContainsAsyncWithLibusbTransfer(const libusb_transfer* transfer) const;

  // These getters return copies, to avoid threading issues.
  Info GetByAsyncRequestState(
      const TransferAsyncRequestState* async_request_state) const;
  Info GetAsyncByDestination(
      const UsbTransferDestination& transfer_destination) const;
  Info GetAsyncByLibusbTransfer(const libusb_transfer* transfer) const;
  // Returns the transfer with the minimum `timeout` value.
  Info GetWithMinTimeout() const;

  // Moves and returns `prepared_js_call` for an in-flight transfer with the
  // specified destination, if there's any.
  optional<RemoteCallAsyncRequest> ExtractPreparedJsCall(
      const UsbTransferDestination& transfer_destination);

  void RemoveByAsyncRequestState(
      const TransferAsyncRequestState* async_request_state);

 private:
  // Holds `Info` and all related non-public fields.
  struct Item {
    Info info;
    optional<RemoteCallAsyncRequest> prepared_js_call;
  };

  template <typename Key>
  Item* FindItem(const Key& mapping_key,
                 const std::map<Key, Item*>& mapping) const;
  template <typename Key>
  Item* GetFifoItem(const Key& mapping_key,
                    const std::map<Key, std::deque<Item*>>& mapping) const;
  template <typename Key>
  bool RemoveItemFromMappedQueue(Item* item,
                                 const Key& mapping_key,
                                 std::map<Key, std::deque<Item*>>& mapping);

  void Add(std::unique_ptr<Item> item);
  void Remove(Item* item);

  mutable std::mutex mutex_;
  // Storage that owns `Item` objects.
  std::map<const Item*, std::unique_ptr<Item>> items_;
  // Mappings from various keys to `Item` pointers. We use `std::deque` to
  // allow getters pick up transfers in the FIFO order.
  std::map<const TransferAsyncRequestState*, Item*>
      async_request_state_mapping_;
  std::map<UsbTransferDestination, std::deque<Item*>>
      async_destination_mapping_;
  std::map<const libusb_transfer*, Item*> async_libusb_transfer_mapping_;
  std::map<std::chrono::time_point<std::chrono::high_resolution_clock>,
           std::deque<Item*>>
      timeout_mapping_;
  // Contains items which still have nonempty `prepared_js_call`.
  std::map<UsbTransferDestination, std::deque<Item*>>
      transfers_with_prepared_js_call_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_USB_TRANSFERS_PARAMETERS_STORAGE_H_
