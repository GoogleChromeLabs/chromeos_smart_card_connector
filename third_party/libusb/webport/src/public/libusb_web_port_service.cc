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

#include "third_party/libusb/webport/src/public/libusb_web_port_service.h"

#include <utility>

#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/requesting/js_requester.h"
#include "common/cpp/src/public/requesting/requester.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "third_party/libusb/webport/src/libusb_interface.h"
#include "third_party/libusb/webport/src/libusb_js_proxy.h"
#include "third_party/libusb/webport/src/libusb_tracing_wrapper.h"

namespace {

// A unique global pointer to an implementation of LibusbInterface interface
// that is used by the implementation of libusb_* functions in this library.
google_smart_card::LibusbInterface* g_libusb = nullptr;

google_smart_card::LibusbInterface* GetLibusbImpl() {
  GOOGLE_SMART_CARD_CHECK(g_libusb);
  return g_libusb;
}

}  // namespace

namespace google_smart_card {

class LibusbWebPortService::Impl final {
 public:
  Impl(GlobalContext* global_context, TypedMessageRouter* typed_message_router)
      : libusb_js_proxy_(global_context, typed_message_router) {
#ifndef NDEBUG
    libusb_tracing_wrapper_.reset(new LibusbTracingWrapper(&libusb_js_proxy_));
#endif  // NDEBUG
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  ~Impl() = default;

  void ShutDown() { libusb_js_proxy_.ShutDown(); }

  LibusbInterface* libusb() {
    if (libusb_tracing_wrapper_)
      return libusb_tracing_wrapper_.get();
    return &libusb_js_proxy_;
  }

 private:
  LibusbJsProxy libusb_js_proxy_;
  std::unique_ptr<LibusbTracingWrapper> libusb_tracing_wrapper_;
};

LibusbWebPortService::LibusbWebPortService(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router)
    : impl_(MakeUnique<Impl>(global_context, typed_message_router)) {
  GOOGLE_SMART_CARD_CHECK(!g_libusb);
  g_libusb = impl_->libusb();
}

LibusbWebPortService::~LibusbWebPortService() {
  GOOGLE_SMART_CARD_CHECK(g_libusb == impl_->libusb());
  g_libusb = nullptr;
}

void LibusbWebPortService::ShutDown() {
  impl_->ShutDown();
}

}  // namespace google_smart_card

int LIBUSB_CALL libusb_init(libusb_context** ctx) {
  return GetLibusbImpl()->LibusbInit(ctx);
}

void LIBUSB_CALL libusb_exit(libusb_context* ctx) {
  GetLibusbImpl()->LibusbExit(ctx);
}

ssize_t LIBUSB_CALL libusb_get_device_list(libusb_context* ctx,
                                           libusb_device*** list) {
  return GetLibusbImpl()->LibusbGetDeviceList(ctx, list);
}

void LIBUSB_CALL libusb_free_device_list(libusb_device** list,
                                         int unref_devices) {
  GetLibusbImpl()->LibusbFreeDeviceList(list, unref_devices);
}

libusb_device* LIBUSB_CALL libusb_ref_device(libusb_device* dev) {
  return GetLibusbImpl()->LibusbRefDevice(dev);
}

void LIBUSB_CALL libusb_unref_device(libusb_device* dev) {
  GetLibusbImpl()->LibusbUnrefDevice(dev);
}

int LIBUSB_CALL
libusb_get_active_config_descriptor(libusb_device* dev,
                                    libusb_config_descriptor** config) {
  return GetLibusbImpl()->LibusbGetActiveConfigDescriptor(dev, config);
}

void LIBUSB_CALL
libusb_free_config_descriptor(libusb_config_descriptor* config) {
  return GetLibusbImpl()->LibusbFreeConfigDescriptor(config);
}

int LIBUSB_CALL libusb_get_device_descriptor(libusb_device* dev,
                                             libusb_device_descriptor* desc) {
  return GetLibusbImpl()->LibusbGetDeviceDescriptor(dev, desc);
}

uint8_t LIBUSB_CALL libusb_get_bus_number(libusb_device* dev) {
  return GetLibusbImpl()->LibusbGetBusNumber(dev);
}

uint8_t LIBUSB_CALL libusb_get_device_address(libusb_device* dev) {
  return GetLibusbImpl()->LibusbGetDeviceAddress(dev);
}

int LIBUSB_CALL libusb_open(libusb_device* dev, libusb_device_handle** handle) {
  return GetLibusbImpl()->LibusbOpen(dev, handle);
}

void LIBUSB_CALL libusb_close(libusb_device_handle* dev_handle) {
  return GetLibusbImpl()->LibusbClose(dev_handle);
}

int LIBUSB_CALL libusb_claim_interface(libusb_device_handle* dev,
                                       int interface_number) {
  return GetLibusbImpl()->LibusbClaimInterface(dev, interface_number);
}

int LIBUSB_CALL libusb_release_interface(libusb_device_handle* dev,
                                         int interface_number) {
  return GetLibusbImpl()->LibusbReleaseInterface(dev, interface_number);
}

libusb_transfer* LIBUSB_CALL libusb_alloc_transfer(int iso_packets) {
  return GetLibusbImpl()->LibusbAllocTransfer(iso_packets);
}

int LIBUSB_CALL libusb_submit_transfer(libusb_transfer* transfer) {
  return GetLibusbImpl()->LibusbSubmitTransfer(transfer);
}

int LIBUSB_CALL libusb_cancel_transfer(libusb_transfer* transfer) {
  return GetLibusbImpl()->LibusbCancelTransfer(transfer);
}

void LIBUSB_CALL libusb_free_transfer(libusb_transfer* transfer) {
  GetLibusbImpl()->LibusbFreeTransfer(transfer);
}

int LIBUSB_CALL libusb_reset_device(libusb_device_handle* dev) {
  return GetLibusbImpl()->LibusbResetDevice(dev);
}

int LIBUSB_CALL libusb_control_transfer(libusb_device_handle* dev_handle,
                                        uint8_t request_type,
                                        uint8_t bRequest,
                                        uint16_t wValue,
                                        uint16_t wIndex,
                                        unsigned char* data,
                                        uint16_t wLength,
                                        unsigned int timeout) {
  return GetLibusbImpl()->LibusbControlTransfer(dev_handle, request_type,
                                                bRequest, wValue, wIndex, data,
                                                wLength, timeout);
}

int LIBUSB_CALL libusb_bulk_transfer(libusb_device_handle* dev_handle,
                                     unsigned char endpoint,
                                     unsigned char* data,
                                     int length,
                                     int* actual_length,
                                     unsigned int timeout) {
  return GetLibusbImpl()->LibusbBulkTransfer(dev_handle, endpoint, data, length,
                                             actual_length, timeout);
}

int LIBUSB_CALL libusb_interrupt_transfer(libusb_device_handle* dev_handle,
                                          unsigned char endpoint,
                                          unsigned char* data,
                                          int length,
                                          int* actual_length,
                                          unsigned int timeout) {
  return GetLibusbImpl()->LibusbInterruptTransfer(
      dev_handle, endpoint, data, length, actual_length, timeout);
}

int LIBUSB_CALL libusb_handle_events(libusb_context* ctx) {
  return GetLibusbImpl()->LibusbHandleEvents(ctx);
}

int LIBUSB_CALL libusb_handle_events_completed(libusb_context* ctx,
                                               int* completed) {
  return GetLibusbImpl()->LibusbHandleEventsCompleted(ctx, completed);
}
