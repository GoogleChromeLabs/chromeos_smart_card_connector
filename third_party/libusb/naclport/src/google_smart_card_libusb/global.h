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

#ifndef GOOGLE_SMART_CARD_LIBUSB_GLOBAL_H_
#define GOOGLE_SMART_CARD_LIBUSB_GLOBAL_H_

#include <memory>

#include <ppapi/cpp/core.h>
#include <ppapi/cpp/instance.h>

#include <google_smart_card_common/messaging/typed_message_router.h>

namespace google_smart_card {

// This class owns a LibusbOverChromeUsb instance and enables it to be used by
// the implementation of the global libusb_* functions.
//
// All global libusb_* functions are allowed to be called only while the
// LibusbOverChromeUsbGlobal object exists.
//
// It is allowed to have at most one LibusbOverChromeUsbGlobal constructed at
// any given moment of time.
//
// Note: The class constructor and destructor are not thread-safe against any
// concurrent libusb_* function calls.
class LibusbOverChromeUsbGlobal final {
 public:
  LibusbOverChromeUsbGlobal(
      TypedMessageRouter* typed_message_router,
      pp::Instance* pp_instance,
      pp::Core* pp_core);

  LibusbOverChromeUsbGlobal(const LibusbOverChromeUsbGlobal&) = delete;

  // Destroys the self instance and the owned LibusbOverChromeUsb instance.
  //
  // After the destructor is called, any global libusb_* function calls are not
  // allowed (and the still running calls, if any, will introduce undefined
  // behavior).
  ~LibusbOverChromeUsbGlobal();

  // Detaches from the Pepper module and the typed message router, which
  // prevents making any further requests through them and prevents waiting for
  // the responses of the already started requests.
  //
  // After this function call, the global libusb_* functions are still allowed
  // to be called, but they will return errors instead of performing the real
  // requests.
  //
  // This function is primarily intended to be used during the Pepper module
  // shutdown process, for preventing the situations when some other threads
  // currently calling global libusb_* functions or waiting for the finish of
  // the already called functions try to access the destroyed pp::Instance
  // object or some other associated objects.
  //
  // This function is safe to be called from any thread.
  void Detach();

 private:
  class Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_LIBUSB_GLOBAL_H_
