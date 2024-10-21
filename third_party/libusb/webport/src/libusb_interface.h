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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_INTERFACE_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_INTERFACE_H_

#include <stdint.h>

#include <libusb.h>

namespace google_smart_card {

// Interface corresponding to the libusb API.
//
// All functions presented here have the same signatures as the original
// libusb API functions (see the libusb.h header in the original libusb sources
// and the documentation at
// <http://libusb.sourceforge.net/api-1.0/api.html>).
class LibusbInterface {
 public:
  virtual ~LibusbInterface() = default;

  virtual int LibusbInit(libusb_context** ctx) = 0;
  virtual void LibusbExit(libusb_context* ctx) = 0;

  virtual ssize_t LibusbGetDeviceList(libusb_context* ctx,
                                      libusb_device*** list) = 0;
  virtual void LibusbFreeDeviceList(libusb_device** list,
                                    int unref_devices) = 0;

  virtual libusb_device* LibusbRefDevice(libusb_device* dev) = 0;
  virtual void LibusbUnrefDevice(libusb_device* dev) = 0;

  virtual int LibusbGetActiveConfigDescriptor(
      libusb_device* dev,
      libusb_config_descriptor** config) = 0;
  virtual void LibusbFreeConfigDescriptor(libusb_config_descriptor* config) = 0;

  virtual int LibusbGetDeviceDescriptor(libusb_device* dev,
                                        libusb_device_descriptor* desc) = 0;

  virtual uint8_t LibusbGetBusNumber(libusb_device* dev) = 0;
  virtual uint8_t LibusbGetDeviceAddress(libusb_device* dev) = 0;

  virtual int LibusbOpen(libusb_device* dev, libusb_device_handle** handle) = 0;
  virtual libusb_device_handle* LibusbOpenDeviceWithVidPid(
      libusb_context* ctx,
      uint16_t vendor_id,
      uint16_t product_id) = 0;
  virtual void LibusbClose(libusb_device_handle* handle) = 0;

  virtual libusb_device* LibusbGetDevice(libusb_device_handle* dev_handle) = 0;

  virtual int LibusbClaimInterface(libusb_device_handle* dev,
                                   int interface_number) = 0;
  virtual int LibusbReleaseInterface(libusb_device_handle* dev,
                                     int interface_number) = 0;

  virtual int LibusbResetDevice(libusb_device_handle* dev) = 0;

  virtual libusb_transfer* LibusbAllocTransfer(int iso_packets) = 0;
  virtual int LibusbSubmitTransfer(libusb_transfer* transfer) = 0;
  virtual int LibusbCancelTransfer(libusb_transfer* transfer) = 0;
  virtual void LibusbFreeTransfer(libusb_transfer* transfer) = 0;

  virtual int LibusbControlTransfer(libusb_device_handle* dev,
                                    uint8_t bmRequestType,
                                    uint8_t bRequest,
                                    uint16_t wValue,
                                    uint16_t wIndex,
                                    unsigned char* data,
                                    uint16_t wLength,
                                    unsigned timeout) = 0;
  virtual int LibusbBulkTransfer(libusb_device_handle* dev,
                                 unsigned char endpoint,
                                 unsigned char* data,
                                 int length,
                                 int* actual_length,
                                 unsigned timeout) = 0;
  virtual int LibusbInterruptTransfer(libusb_device_handle* dev,
                                      unsigned char endpoint,
                                      unsigned char* data,
                                      int length,
                                      int* actual_length,
                                      unsigned timeout) = 0;

  virtual int LibusbHandleEvents(libusb_context* ctx) = 0;
  virtual int LibusbHandleEventsCompleted(libusb_context* ctx,
                                          int* completed) = 0;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_INTERFACE_H_
