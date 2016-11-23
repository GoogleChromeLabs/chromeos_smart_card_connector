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

#include <google_smart_card_libusb/global.h>

#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/requesting/js_requester.h>
#include <google_smart_card_common/requesting/requester.h>
#include <google_smart_card_common/unique_ptr_utils.h>

#include "chrome_usb/api_bridge.h"
#include "libusb_interface.h"
#include "libusb_over_chrome_usb.h"
#include "libusb_tracing_wrapper.h"

namespace {

// A unique global pointer to an implementation of LibusbInterface interface
// that is used by the implementation of libusb_* functions in this library.
google_smart_card::LibusbInterface* g_libusb = nullptr;

google_smart_card::LibusbInterface* GetGlobalLibusb() {
  GOOGLE_SMART_CARD_CHECK(g_libusb);
  return g_libusb;
}

}  // namespace

namespace google_smart_card {

class LibusbOverChromeUsbGlobal::Impl final {
 public:
  Impl(
      TypedMessageRouter* typed_message_router,
      pp::Instance* pp_instance,
      pp::Core* pp_core)
      : chrome_usb_api_bridge_(MakeRequester(
            typed_message_router, pp_instance, pp_core)),
        libusb_over_chrome_usb_(&chrome_usb_api_bridge_) {
#ifndef NDEBUG
    libusb_tracing_wrapper_.reset(new LibusbTracingWrapper(
        &libusb_over_chrome_usb_));
#endif  // NDEBUG
  }

  Impl(const Impl&) = delete;

  void Detach() {
    chrome_usb_api_bridge_.Detach();
  }

  LibusbInterface* libusb() {
    if (libusb_tracing_wrapper_)
      return libusb_tracing_wrapper_.get();
    return &libusb_over_chrome_usb_;
  }

 private:
  static std::unique_ptr<Requester> MakeRequester(
      TypedMessageRouter* typed_message_router,
      pp::Instance* pp_instance,
      pp::Core* pp_core) {
    return std::unique_ptr<Requester>(new JsRequester(
        chrome_usb::kApiBridgeRequesterName,
        typed_message_router,
        MakeUnique<JsRequester::PpDelegateImpl>(pp_instance, pp_core)));
  }

  chrome_usb::ApiBridge chrome_usb_api_bridge_;
  LibusbOverChromeUsb libusb_over_chrome_usb_;
  std::unique_ptr<LibusbTracingWrapper> libusb_tracing_wrapper_;
};

LibusbOverChromeUsbGlobal::LibusbOverChromeUsbGlobal(
    TypedMessageRouter* typed_message_router,
    pp::Instance* pp_instance,
    pp::Core* pp_core)
    : impl_(new Impl(typed_message_router, pp_instance, pp_core)) {
  GOOGLE_SMART_CARD_CHECK(!g_libusb);
  g_libusb = impl_->libusb();
}

LibusbOverChromeUsbGlobal::~LibusbOverChromeUsbGlobal() {
  GOOGLE_SMART_CARD_CHECK(g_libusb == impl_->libusb());
  g_libusb = nullptr;
}

void LibusbOverChromeUsbGlobal::Detach() {
  impl_->Detach();
}

}  // namespace google_smart_card

int LIBUSB_CALL libusb_init(libusb_context** ctx) {
  return GetGlobalLibusb()->LibusbInit(ctx);
}

void LIBUSB_CALL libusb_exit(libusb_context* ctx) {
  GetGlobalLibusb()->LibusbExit(ctx);
}

ssize_t LIBUSB_CALL libusb_get_device_list(
    libusb_context* ctx, libusb_device*** list) {
  return GetGlobalLibusb()->LibusbGetDeviceList(ctx, list);
}

void LIBUSB_CALL libusb_free_device_list(
    libusb_device** list, int unref_devices) {
  GetGlobalLibusb()->LibusbFreeDeviceList(list, unref_devices);
}

libusb_device* LIBUSB_CALL libusb_ref_device(libusb_device* dev) {
  return GetGlobalLibusb()->LibusbRefDevice(dev);
}

void LIBUSB_CALL libusb_unref_device(libusb_device* dev) {
  GetGlobalLibusb()->LibusbUnrefDevice(dev);
}

int LIBUSB_CALL libusb_get_active_config_descriptor(
    libusb_device* dev, libusb_config_descriptor** config) {
  return GetGlobalLibusb()->LibusbGetActiveConfigDescriptor(dev, config);
}

void LIBUSB_CALL libusb_free_config_descriptor(
    libusb_config_descriptor* config) {
  return GetGlobalLibusb()->LibusbFreeConfigDescriptor(config);
}

int LIBUSB_CALL libusb_get_device_descriptor(
    libusb_device* dev, libusb_device_descriptor* desc) {
  return GetGlobalLibusb()->LibusbGetDeviceDescriptor(dev, desc);
}

uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device* dev) {
  return GetGlobalLibusb()->LibusbGetBusNumber(dev);
}

uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device* dev) {
  return GetGlobalLibusb()->LibusbGetDeviceAddress(dev);
}

int LIBUSB_CALL libusb_open(libusb_device* dev, libusb_device_handle** handle) {
  return GetGlobalLibusb()->LibusbOpen(dev, handle);
}

void LIBUSB_CALL libusb_close(libusb_device_handle* dev_handle) {
  return GetGlobalLibusb()->LibusbClose(dev_handle);
}

int LIBUSB_CALL libusb_claim_interface(
    libusb_device_handle* dev, int interface_number) {
  return GetGlobalLibusb()->LibusbClaimInterface(dev, interface_number);
}

int LIBUSB_CALL libusb_release_interface(
    libusb_device_handle* dev, int interface_number) {
  return GetGlobalLibusb()->LibusbReleaseInterface(dev, interface_number);
}

libusb_transfer* LIBUSB_CALL libusb_alloc_transfer(int iso_packets) {
  return GetGlobalLibusb()->LibusbAllocTransfer(iso_packets);
}

int LIBUSB_CALL libusb_submit_transfer(libusb_transfer* transfer) {
  return GetGlobalLibusb()->LibusbSubmitTransfer(transfer);
}

int LIBUSB_CALL libusb_cancel_transfer(libusb_transfer* transfer) {
  return GetGlobalLibusb()->LibusbCancelTransfer(transfer);
}

void LIBUSB_CALL libusb_free_transfer(libusb_transfer* transfer) {
  GetGlobalLibusb()->LibusbFreeTransfer(transfer);
}

int LIBUSB_CALL libusb_reset_device(libusb_device_handle* dev) {
  return GetGlobalLibusb()->LibusbResetDevice(dev);
}

int LIBUSB_CALL libusb_control_transfer(
    libusb_device_handle* dev_handle,
    uint8_t request_type,
    uint8_t bRequest,
    uint16_t wValue,
    uint16_t wIndex,
    unsigned char* data,
    uint16_t wLength,
    unsigned int timeout) {
  return GetGlobalLibusb()->LibusbControlTransfer(
      dev_handle,
      request_type,
      bRequest,
      wValue,
      wIndex,
      data,
      wLength,
      timeout);
}

int LIBUSB_CALL libusb_bulk_transfer(
    libusb_device_handle* dev_handle,
    unsigned char endpoint,
    unsigned char* data,
    int length,
    int* actual_length,
    unsigned int timeout) {
  return GetGlobalLibusb()->LibusbBulkTransfer(
      dev_handle, endpoint, data, length, actual_length, timeout);
}

int LIBUSB_CALL libusb_interrupt_transfer(
    libusb_device_handle* dev_handle,
    unsigned char endpoint,
    unsigned char* data,
    int length,
    int* actual_length,
    unsigned int timeout) {
  return GetGlobalLibusb()->LibusbInterruptTransfer(
      dev_handle, endpoint, data, length, actual_length, timeout);
}

int LIBUSB_CALL libusb_handle_events(libusb_context* ctx) {
  return GetGlobalLibusb()->LibusbHandleEvents(ctx);
}

int LIBUSB_CALL libusb_handle_events_completed(
    libusb_context* ctx, int* completed) {
  return GetGlobalLibusb()->LibusbHandleEventsCompleted(ctx, completed);
}
