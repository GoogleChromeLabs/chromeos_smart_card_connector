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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_CONTEXTS_STORAGE_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_CONTEXTS_STORAGE_H_

#include <memory>
#include <mutex>
#include <unordered_map>

#include <libusb.h>

#include "libusb_opaque_types.h"

namespace google_smart_card {

// This class provides a thread-safe storage of libusb_context instances.
//
// The main reason for using this class is that all libusb_* functions operate
// only with the raw pointers, while in this libusb port the libusb_context
// instances have to be stored in ref-counted pointers (for the reasoning, refer
// to the libusb_opaque_types.h header).
class LibusbContextsStorage final {
 public:
  LibusbContextsStorage() = default;
  LibusbContextsStorage(const LibusbContextsStorage&) = delete;

  std::shared_ptr<libusb_context> CreateContext();

  void DestroyContext(const libusb_context* context);

  std::shared_ptr<libusb_context> FindContextByAddress(
      const libusb_context* context) const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<const libusb_context*, std::shared_ptr<libusb_context>>
  mapping_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_LIBUSB_LIBUSB_CONTEXTS_STORAGE_H_
