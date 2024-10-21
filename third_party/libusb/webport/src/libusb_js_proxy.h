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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_JS_PROXY_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_JS_PROXY_H_

#include <stdint.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include <libusb.h>

#include "common/cpp/src/public/global_context.h"
#include "common/cpp/src/public/messaging/typed_message_router.h"
#include "common/cpp/src/public/requesting/js_requester.h"
#include "common/cpp/src/public/requesting/remote_call_adaptor.h"
#include "common/cpp/src/public/requesting/remote_call_async_request.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "third_party/libusb/webport/src/libusb_contexts_storage.h"
#include "third_party/libusb/webport/src/libusb_interface.h"
#include "third_party/libusb/webport/src/libusb_opaque_types.h"
#include "third_party/libusb/webport/src/usb_transfer_destination.h"

namespace google_smart_card {

// This class provides an implementation for the libusb library interface that
// forwards all requests to a JavaScript API (e.g., WebUSB or chrome.usb).
//
// See libusb-proxy-receiver.js for the JavaScript counterpart implementation.
class LibusbJsProxy final : public LibusbInterface {
 public:
  using TransferRequestResult = RequestResult<LibusbJsTransferResult>;
  using TransferAsyncRequestState = AsyncRequestState<LibusbJsTransferResult>;
  using TransferAsyncRequestStatePtr =
      std::shared_ptr<TransferAsyncRequestState>;
  using TransferAsyncRequestCallback =
      AsyncRequestCallback<LibusbJsTransferResult>;

  LibusbJsProxy(GlobalContext* global_context,
                TypedMessageRouter* typed_message_router);
  LibusbJsProxy(const LibusbJsProxy&) = delete;
  LibusbJsProxy& operator=(const LibusbJsProxy&) = delete;
  ~LibusbJsProxy() override;

  // Detaches from the typed message router and the JavaScript side, which
  // prevents making any further requests and prevents waiting for the responses
  // of the already started requests.
  //
  // After this function call, all methods are still allowed to be called, but
  // they will return errors instead of performing the real requests.
  //
  // This function is primarily intended to be used during the executable
  // shutdown process, for preventing the situations when some other threads
  // currently executing global libusb operations would trigger accesses to
  // already destroyed objects.
  //
  // This function is safe to be called from any thread.
  void ShutDown();

  // LibusbInterface:
  int LibusbInit(libusb_context** ctx) override;
  void LibusbExit(libusb_context* ctx) override;
  ssize_t LibusbGetDeviceList(libusb_context* ctx,
                              libusb_device*** list) override;
  void LibusbFreeDeviceList(libusb_device** list, int unref_devices) override;
  libusb_device* LibusbRefDevice(libusb_device* dev) override;
  void LibusbUnrefDevice(libusb_device* dev) override;
  int LibusbGetActiveConfigDescriptor(
      libusb_device* dev,
      libusb_config_descriptor** config) override;
  void LibusbFreeConfigDescriptor(libusb_config_descriptor* config) override;
  int LibusbGetDeviceDescriptor(libusb_device* dev,
                                libusb_device_descriptor* desc) override;
  uint8_t LibusbGetBusNumber(libusb_device* dev) override;
  uint8_t LibusbGetDeviceAddress(libusb_device* dev) override;
  int LibusbOpen(libusb_device* dev, libusb_device_handle** handle) override;
  libusb_device_handle* LibusbOpenDeviceWithVidPid(
      libusb_context* ctx,
      uint16_t vendor_id,
      uint16_t product_id) override;
  void LibusbClose(libusb_device_handle* handle) override;
  libusb_device* LibusbGetDevice(libusb_device_handle* dev_handle) override;
  int LibusbClaimInterface(libusb_device_handle* dev,
                           int interface_number) override;
  int LibusbReleaseInterface(libusb_device_handle* dev,
                             int interface_number) override;
  int LibusbResetDevice(libusb_device_handle* dev) override;
  libusb_transfer* LibusbAllocTransfer(int iso_packets) override;
  int LibusbSubmitTransfer(libusb_transfer* transfer) override;
  int LibusbCancelTransfer(libusb_transfer* transfer) override;
  void LibusbFreeTransfer(libusb_transfer* transfer) override;
  int LibusbControlTransfer(libusb_device_handle* dev,
                            uint8_t bmRequestType,
                            uint8_t bRequest,
                            uint16_t wValue,
                            uint16_t wIndex,
                            unsigned char* data,
                            uint16_t wLength,
                            unsigned timeout) override;
  int LibusbBulkTransfer(libusb_device_handle* dev,
                         unsigned char endpoint,
                         unsigned char* data,
                         int length,
                         int* actual_length,
                         unsigned timeout) override;
  int LibusbInterruptTransfer(libusb_device_handle* dev,
                              unsigned char endpoint,
                              unsigned char* data,
                              int length,
                              int* actual_length,
                              unsigned timeout) override;
  int LibusbHandleEvents(libusb_context* ctx) override;
  int LibusbHandleEventsCompleted(libusb_context* ctx, int* completed) override;

  // Makes the bus number to be reported as the specified value for the given
  // device.
  void OverrideBusNumber(int64_t device_address, uint8_t new_bus_number);

 private:
  const int kHandleEventsTimeoutSeconds = 60;

  libusb_context* SubstituteDefaultContextIfNull(
      libusb_context* context_or_nullptr) const;
  TransferAsyncRequestCallback WrapLibusbTransferCallback(
      libusb_transfer* transfer);
  int LibusbHandleEventsWithTimeout(libusb_context* context,
                                    int timeout_seconds);
  std::function<void(GenericRequestResult)> MakeLibusbJsTransferCallback(
      const std::string& js_api_name,
      std::weak_ptr<libusb_context> context,
      const UsbTransferDestination& transfer_destination,
      TransferAsyncRequestStatePtr async_request_state);
  RemoteCallAsyncRequest PrepareTransferJsApiCall(
      libusb_context* const context,
      libusb_transfer* transfer,
      const UsbTransferDestination& transfer_destination,
      std::shared_ptr<TransferAsyncRequestState> async_request_state);
  void MaybeStartTransferJsRequest(
      libusb_context* const context,
      const UsbTransferDestination& transfer_destination);
  int DoGenericSyncTranfer(libusb_transfer_type transfer_type,
                           libusb_device_handle* device_handle,
                           unsigned char endpoint_address,
                           unsigned char* data,
                           int length,
                           int* actual_length,
                           unsigned timeout);
  // Fetches the descriptor from the JS side and stores it in
  // `libusb_device::js_config`.
  void ObtainActiveConfigDescriptor(libusb_device* dev);

  // Helpers for making requests to the JavaScript side.
  JsRequester js_requester_;
  RemoteCallAdaptor js_call_adaptor_;

  // Map that holds the (fake) bus number for each device. Keys are
  // `libusb_device->js_device().device_id`. For newly added devices the
  // `kDefaultBusNumber` value is used.
  std::unordered_map<int64_t, uint8_t> bus_numbers_;

  LibusbContextsStorage contexts_storage_;
  const std::shared_ptr<libusb_context> default_context_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_JS_PROXY_H_
