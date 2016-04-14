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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_OVER_CHROME_USB_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_OVER_CHROME_USB_H_

#include <stdint.h>

#include <memory>

#include <google_smart_card_common/requesting/request_result.h>

#include "libusb.h"

#include "chrome_usb/api_bridge.h"
#include "chrome_usb/types.h"

namespace google_smart_card {

// This class provides an implementation for the libusb library interface that
// forwards all requests to the chrome.usb JavaScript API (see
// <https://developer.chrome.com/apps/usb>).
//
// For the details of the integration with the chrome.usb JavaScript API, see
// the chrome_usb/api_bridge.h file.
class LibusbOverChromeUsb final {
 public:
  explicit LibusbOverChromeUsb(chrome_usb::ApiBridge* chrome_usb_api_bridge);

  ~LibusbOverChromeUsb();

  int LibusbInit(libusb_context** context);
  void LibusbExit(libusb_context* context);

  ssize_t LibusbGetDeviceList(
      libusb_context* context, libusb_device*** device_list);
  void LibusbFreeDeviceList(libusb_device** device_list, int unref_devices);

  libusb_device* LibusbRefDevice(libusb_device* device);
  void LibusbUnrefDevice(libusb_device* device);

  int LibusbGetActiveConfigDescriptor(
      libusb_device* device, libusb_config_descriptor** config_descriptor);
  void LibusbFreeConfigDescriptor(libusb_config_descriptor* config_descriptor);

  int LibusbGetDeviceDescriptor(
      libusb_device* device, libusb_device_descriptor* device_descriptor);

  uint8_t LibusbGetBusNumber(libusb_device* device);
  uint8_t LibusbGetDeviceAddress(libusb_device* device);

  int LibusbOpen(libusb_device* device, libusb_device_handle** device_handle);
  void LibusbClose(libusb_device_handle* device_handle);

  int LibusbClaimInterface(
      libusb_device_handle* device_handle, int interface_number);
  int LibusbReleaseInterface(
      libusb_device_handle* device_handle, int interface_number);

  int LibusbResetDevice(libusb_device_handle* device_handle);

  libusb_transfer* LibusbAllocTransfer(int isochronous_packet_count) const;
  int LibusbSubmitTransfer(libusb_transfer* transfer);
  int LibusbCancelTransfer(libusb_transfer* transfer);
  void LibusbFreeTransfer(libusb_transfer* transfer) const;

  int LibusbControlTransfer(
      libusb_device_handle* device_handle,
      uint8_t request_type,
      uint8_t request,
      uint16_t value,
      uint16_t index,
      unsigned char* data,
      uint16_t length,
      unsigned timeout);
  int LibusbBulkTransfer(
      libusb_device_handle* device_handle,
      unsigned char endpoint_address,
      unsigned char* data,
      int length,
      int* actual_length,
      unsigned timeout);
  int LibusbInterruptTransfer(
      libusb_device_handle* device_handle,
      unsigned char endpoint_address,
      unsigned char* data,
      int length,
      int* actual_length,
      unsigned timeout);

  int LibusbHandleEvents(libusb_context* context);
  int LibusbHandleEventsTimeout(libusb_context* context, int timeout_seconds);

 private:
  const int kHandleEventsDefaultTimeoutSeconds = 60;

  libusb_context* SubstituteDefaultContextIfNull(
      libusb_context* context_or_nullptr) const;
  libusb_context* GetLibusbTransferContext(
      const libusb_transfer* transfer) const;
  libusb_context* GetLibusbTransferContextChecked(
      const libusb_transfer* transfer) const;
  chrome_usb::AsyncTransferCallback MakeAsyncTransferCallback(
      libusb_transfer* transfer) const;
  void ProcessCompletedAsyncTransfer(
    libusb_transfer* transfer,
    RequestResult<chrome_usb::TransferResult> request_result) const;

  chrome_usb::ApiBridge* const chrome_usb_api_bridge_;
  const std::unique_ptr<libusb_context> default_context_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_OVER_CHROME_USB_H_
