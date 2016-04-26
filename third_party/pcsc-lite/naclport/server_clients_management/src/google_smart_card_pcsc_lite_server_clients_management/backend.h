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

#ifndef GOOGLE_SMART_CARD_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_BACKEND_H_
#define GOOGLE_SMART_CARD_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_BACKEND_H_

#include <memory>

#include <ppapi/cpp/instance.h>

#include <google_smart_card_common/messaging/typed_message_router.h>

namespace google_smart_card {

// The class that enables the PC/SC-Lite server clients management (i.e.
// adding/removing of PC/SC-Lite clients and performing PC/SC-Lite requests
// received from them).
//
// The clients management is enabled in the constructor by adding several typed
// message handlers into the passed typed message router.
//
// Note: clients of the class should ensure that no corresponding incoming
// messages arrive simultaneously with the class destruction.
class PcscLiteServerClientsManagementBackend final {
 public:
  PcscLiteServerClientsManagementBackend(
      pp::Instance* pp_instance, TypedMessageRouter* typed_message_router);

  ~PcscLiteServerClientsManagementBackend();

 private:
  class Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_PCSC_LITE_SERVER_CLIENTS_MANAGEMENT_BACKEND_H_
