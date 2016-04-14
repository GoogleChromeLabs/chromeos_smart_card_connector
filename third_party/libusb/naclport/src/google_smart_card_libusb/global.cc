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

// FIXME(emaxx): Replace functions call tracing through manual logging in this
// file with the features provided by the
// google_smart_card_common/logging/function_call_tracer.h library.

#include <google_smart_card_libusb/global.h>

#include <utility>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/requesting/js_requester.h>
#include <google_smart_card_common/requesting/requester.h>
#include <google_smart_card_common/unique_ptr_utils.h>

#include "chrome_usb/api_bridge.h"
#include "libusb_over_chrome_usb.h"

namespace {

// A unique global instance of LibusbOverChromeUsb class that is used by the
// implementation of libusb_* functions in this library.
google_smart_card::LibusbOverChromeUsb* g_libusb_over_chrome_usb = nullptr;

google_smart_card::LibusbOverChromeUsb* GetGlobalLibusbOverChromeUsb() {
  GOOGLE_SMART_CARD_CHECK(g_libusb_over_chrome_usb);
  return g_libusb_over_chrome_usb;
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
        libusb_over_chrome_usb_(&chrome_usb_api_bridge_) {}

  void Detach() {
    chrome_usb_api_bridge_.Detach();
  }

  LibusbOverChromeUsb* libusb_over_chrome_usb() {
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
};

LibusbOverChromeUsbGlobal::LibusbOverChromeUsbGlobal(
    TypedMessageRouter* typed_message_router,
    pp::Instance* pp_instance,
    pp::Core* pp_core)
    : impl_(new Impl(typed_message_router, pp_instance, pp_core)) {
  GOOGLE_SMART_CARD_CHECK(!g_libusb_over_chrome_usb);
  g_libusb_over_chrome_usb = impl_->libusb_over_chrome_usb();
}

LibusbOverChromeUsbGlobal::~LibusbOverChromeUsbGlobal() {
  GOOGLE_SMART_CARD_CHECK(
      g_libusb_over_chrome_usb == impl_->libusb_over_chrome_usb());
  g_libusb_over_chrome_usb = nullptr;
}

void LibusbOverChromeUsbGlobal::Detach() {
  impl_->Detach();
}

}  // namespace google_smart_card

int LIBUSB_CALL libusb_init(libusb_context** ctx) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_init(ctx=" << ctx << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbInit(ctx);
}

void LIBUSB_CALL libusb_exit(libusb_context* ctx) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_exit(ctx=" << ctx << ")";

  GetGlobalLibusbOverChromeUsb()->LibusbExit(ctx);
}

ssize_t LIBUSB_CALL libusb_get_device_list(
    libusb_context* ctx, libusb_device*** list) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_get_device_list(ctx=" << ctx << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbGetDeviceList(ctx, list);
}

void LIBUSB_CALL libusb_free_device_list(
    libusb_device** list, int unref_devices) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_free_device_list(list=" << list << ", unref_devices=" <<
      unref_devices << ")";

  GetGlobalLibusbOverChromeUsb()->LibusbFreeDeviceList(list, unref_devices);
}

libusb_device* LIBUSB_CALL libusb_ref_device(libusb_device* dev) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_ref_device(dev=" << dev << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbRefDevice(dev);
}

void LIBUSB_CALL libusb_unref_device(libusb_device* dev) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_unref_device(dev=" << dev << ")";

  GetGlobalLibusbOverChromeUsb()->LibusbUnrefDevice(dev);
}

int LIBUSB_CALL libusb_get_active_config_descriptor(
    libusb_device* dev, libusb_config_descriptor** config) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_get_active_config_descriptor(dev=" << dev << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbGetActiveConfigDescriptor(
      dev, config);
}

void LIBUSB_CALL libusb_free_config_descriptor(
    libusb_config_descriptor* config) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_free_config_descriptor(config=" << config << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbFreeConfigDescriptor(config);
}

int LIBUSB_CALL libusb_get_device_descriptor(
    libusb_device* dev, libusb_device_descriptor* desc) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_get_device_descriptor(dev=" << dev << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbGetDeviceDescriptor(dev, desc);
}

uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device* dev) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_get_bus_number(dev=" << dev << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbGetBusNumber(dev);
}

uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device* dev) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_get_device_address(dev=" << dev << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbGetDeviceAddress(dev);
}

int LIBUSB_CALL libusb_open(libusb_device* dev, libusb_device_handle** handle) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_open(dev=" << dev << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbOpen(dev, handle);
}

void LIBUSB_CALL libusb_close(libusb_device_handle* dev_handle) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_close(dev_handle=" << dev_handle << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbClose(dev_handle);
}

int LIBUSB_CALL libusb_claim_interface(
    libusb_device_handle* dev, int interface_number) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_claim_interface(dev=" << dev << ", interface_number=" <<
      interface_number << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbClaimInterface(
      dev, interface_number);
}

int LIBUSB_CALL libusb_release_interface(
    libusb_device_handle* dev, int interface_number) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_release_interface(dev=" << dev << ", interface_number=" <<
      interface_number << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbReleaseInterface(
      dev, interface_number);
}

libusb_transfer* LIBUSB_CALL libusb_alloc_transfer(int iso_packets) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_alloc_transfer(iso_packets=" << iso_packets << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbAllocTransfer(iso_packets);
}

int LIBUSB_CALL libusb_submit_transfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_submit_transfer(transfer=" << transfer << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbSubmitTransfer(transfer);
}

int LIBUSB_CALL libusb_cancel_transfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_cancel_transfer(transfer=" << transfer << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbCancelTransfer(transfer);
}

void LIBUSB_CALL libusb_free_transfer(libusb_transfer* transfer) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_free_transfer(transfer=" << transfer << ")";

  GetGlobalLibusbOverChromeUsb()->LibusbFreeTransfer(transfer);
}

int LIBUSB_CALL libusb_reset_device(libusb_device_handle* dev) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_reset_device(dev=" << dev << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbResetDevice(dev);
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
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_control_transfer(dev_handle=" << dev_handle <<
      ", request_type=" << static_cast<int>(request_type) << ", bRequest=" <<
      static_cast<int>(bRequest) << ", wValue=" << wValue << ", wIndex=" <<
      wIndex << ", data=" << static_cast<void*>(data) << ", wLength=" <<
      wLength << ", timeout=" << timeout << ") called";

  return GetGlobalLibusbOverChromeUsb()->LibusbControlTransfer(
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
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_bulk_transfer(dev_handle=" << dev_handle << ", endpoint=" <<
      static_cast<int>(endpoint) << ", data=" << static_cast<void*>(data) <<
      ", length=" << length << ", timeout=" << timeout << ") called";

  return GetGlobalLibusbOverChromeUsb()->LibusbBulkTransfer(
      dev_handle, endpoint, data, length, actual_length, timeout);
}

int LIBUSB_CALL libusb_interrupt_transfer(
    libusb_device_handle* dev_handle,
    unsigned char endpoint,
    unsigned char* data,
    int length,
    int* actual_length,
    unsigned int timeout) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_interrupt_transfer(dev_handle=" << dev_handle << ", endpoint=" <<
      static_cast<int>(endpoint) << ", data=" << static_cast<void*>(data) <<
      ", length=" << length << ", timeout=" << timeout << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbInterruptTransfer(
      dev_handle, endpoint, data, length, actual_length, timeout);
}

int LIBUSB_CALL libusb_handle_events(libusb_context* ctx) {
  GOOGLE_SMART_CARD_LOG_DEBUG << "[libusb NaCl port] " <<
      "libusb_handle_events(ctx=" << ctx << ")";

  return GetGlobalLibusbOverChromeUsb()->LibusbHandleEvents(ctx);
}
