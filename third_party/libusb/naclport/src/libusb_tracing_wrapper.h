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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_TRACING_WRAPPER_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_TRACING_WRAPPER_H_

#include <mutex>
#include <unordered_map>

#include <libusb.h>

#include "libusb_interface.h"

namespace google_smart_card {

// Wrapper that adds debug tracing of the called libusb functions.
//
// Note that the class lifetime should enclose the lifetime of all asynchronous
// libusb requests that were started through it.
class LibusbTracingWrapper : public LibusbInterface {
 public:
  LibusbTracingWrapper(LibusbInterface* wrapped_libusb);
  LibusbTracingWrapper(const LibusbTracingWrapper&) = delete;

  // LibusbInterface:
  int LibusbInit(libusb_context** ctx) override;
  void LibusbExit(libusb_context* ctx) override;
  ssize_t LibusbGetDeviceList(
      libusb_context* ctx, libusb_device*** list) override;
  void LibusbFreeDeviceList(libusb_device** list, int unref_devices) override;
  libusb_device* LibusbRefDevice(libusb_device* dev) override;
  void LibusbUnrefDevice(libusb_device* dev) override;
  int LibusbGetActiveConfigDescriptor(
      libusb_device* dev, libusb_config_descriptor** config) override;
  void LibusbFreeConfigDescriptor(libusb_config_descriptor* config) override;
  int LibusbGetDeviceDescriptor(
      libusb_device* dev, libusb_device_descriptor* desc) override;
  uint8_t LibusbGetBusNumber(libusb_device* dev) override;
  uint8_t LibusbGetDeviceAddress(libusb_device* dev) override;
  int LibusbOpen(libusb_device* dev, libusb_device_handle** handle) override;
  void LibusbClose(libusb_device_handle* handle) override;
  int LibusbClaimInterface(
      libusb_device_handle* dev, int interface_number) override;
  int LibusbReleaseInterface(
      libusb_device_handle* dev, int interface_number) override;
  int LibusbResetDevice(libusb_device_handle* dev) override;
  libusb_transfer* LibusbAllocTransfer(int iso_packets) override;
  int LibusbSubmitTransfer(libusb_transfer* transfer) override;
  int LibusbCancelTransfer(libusb_transfer* transfer) override;
  void LibusbFreeTransfer(libusb_transfer* transfer) override;
  int LibusbControlTransfer(
      libusb_device_handle* dev,
      uint8_t bmRequestType,
      uint8_t bRequest,
      uint16_t wValue,
      uint16_t wIndex,
      unsigned char* data,
      uint16_t wLength,
      unsigned timeout) override;
  int LibusbBulkTransfer(
      libusb_device_handle* dev,
      unsigned char endpoint,
      unsigned char* data,
      int length,
      int* actual_length,
      unsigned timeout) override;
  int LibusbInterruptTransfer(
      libusb_device_handle* dev,
      unsigned char endpoint,
      unsigned char* data,
      int length,
      int* actual_length,
      unsigned timeout) override;
  int LibusbHandleEvents(libusb_context* ctx) override;
  int LibusbHandleEventsCompleted(libusb_context* ctx, int* completed) override;

 private:
  void AddOriginalToWrappedTransferMapItem(
      libusb_transfer* original_transfer, libusb_transfer* wrapped_transfer);
  libusb_transfer* GetWrappedTransfer(libusb_transfer* original_transfer) const;
  void RemoveOriginalToWrappedTransferMapItem(
      libusb_transfer* original_transfer);

  LibusbInterface* const wrapped_libusb_;
  mutable std::mutex mutex_;
  std::unordered_map<libusb_transfer*, libusb_transfer*>
  original_to_wrapped_transfer_map_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_TRACING_WRAPPER_H_
