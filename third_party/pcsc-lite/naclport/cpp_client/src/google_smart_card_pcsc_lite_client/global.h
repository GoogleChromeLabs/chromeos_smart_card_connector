// Copyright 2016 Google Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_SMART_CARD_PCSC_LITE_CLIENT_GLOBAL_H_
#define GOOGLE_SMART_CARD_PCSC_LITE_CLIENT_GLOBAL_H_

#include <memory>

#include <google_smart_card_common/global_context.h>
#include <google_smart_card_common/messaging/typed_message_router.h>

namespace google_smart_card {

// This class owns a PcscLiteOverRequester instance and enables it to be used by
// the implementation of the global PC/SC-Lite client API functions.
//
// All global PC/SC-Lite client API functions are allowed to be called only
// while the PcscLiteOverRequester object exists.
//
// It is allowed to have at most one PcscLiteOverRequester constructed at any
// given moment of time.
//
// Note: Class constructor and destructor are not thread-safe against any
// concurrent PC/SC-Lite client API function calls.
class PcscLiteOverRequesterGlobal final {
 public:
  PcscLiteOverRequesterGlobal(GlobalContext* global_context,
                              TypedMessageRouter* typed_message_router);

  PcscLiteOverRequesterGlobal(const PcscLiteOverRequesterGlobal&) = delete;
  PcscLiteOverRequesterGlobal& operator=(const PcscLiteOverRequesterGlobal&) =
      delete;

  // Destroys the self instance and the owned PcscLiteOverRequester instance.
  //
  // After the destructor is called, any global PC/SC-Lite client API function
  // calls are not allowed (and the still running calls, if any, will introduce
  // undefined behavior).
  ~PcscLiteOverRequesterGlobal();

  // Detaches from the typed message router and the JavaScript side, which
  // prevents making any further requests and prevents waiting for the responses
  // of the already started requests.
  //
  // After this function call, the global PC/SC-Lite client API functions are
  // still allowed to be called, but they will return errors instead of
  // performing the real requests.
  //
  // This function is primarily intended to be used during the executable
  // shutdown process, for preventing the situations when some other threads
  // currently executing global PC/SC-Lite client API functions would trigger
  // accesses to already destroyed objects.
  //
  // This function is safe to be called from any thread.
  void Detach();

 private:
  class Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_PCSC_LITE_CLIENT_GLOBAL_H_
