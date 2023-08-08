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

#ifndef GOOGLE_SMART_CARD_LIBUSB_WEB_PORT_SERVICE_H_
#define GOOGLE_SMART_CARD_LIBUSB_WEB_PORT_SERVICE_H_

#include <memory>

#include "common/cpp/src/google_smart_card_common/global_context.h"
#include "common/cpp/src/google_smart_card_common/messaging/typed_message_router.h"

namespace google_smart_card {

// This class owns a LibusbJsProxy instance and enables it to be used by
// the implementation of the global libusb_* functions.
//
// All global libusb_* functions are allowed to be called only while the
// LibusbWebPortService object exists.
//
// It is allowed to have at most one LibusbWebPortService constructed at any
// given moment of time.
//
// Note: The class constructor and destructor are not thread-safe against any
// concurrent libusb_* function calls.
class LibusbWebPortService final {
 public:
  LibusbWebPortService(GlobalContext* global_context,
                       TypedMessageRouter* typed_message_router);

  LibusbWebPortService(const LibusbWebPortService&) = delete;
  LibusbWebPortService& operator=(const LibusbWebPortService&) = delete;

  // Destroys the self instance and the owned LibusbJsProxy instance.
  //
  // After the destructor is called, any global libusb_* function calls are not
  // allowed (and the still running calls, if any, will introduce undefined
  // behavior).
  ~LibusbWebPortService();

  // Detaches from the typed message router and the JavaScript side, which
  // prevents making any further requests and prevents waiting for the responses
  // of the already started requests.
  //
  // After this function call, the global libusb_* functions are still allowed
  // to be called, but they will return errors instead of performing the real
  // requests.
  //
  // This function is primarily intended to be used during the executable
  // shutdown process, for preventing the situations when some other threads
  // currently executing global libusb_* functions would trigger accesses to
  // already destroyed objects.
  //
  // This function is safe to be called from any thread.
  void ShutDown();

 private:
  // Use the "pimpl" pattern, so that other code can include our .h file without
  // transitively including all of our internal implementation headers (which
  // would break code isolation and also require special compilation flags).
  class Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_LIBUSB_WEB_PORT_SERVICE_H_
