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

#include "third_party/pcsc-lite/webport/server_clients_management/src/google_smart_card_pcsc_lite_server_clients_management/backend.h"

#include "common/cpp/src/public/global_context.h"
#include "third_party/pcsc-lite/webport/server_clients_management/src/clients_manager.h"

namespace google_smart_card {

class PcscLiteServerClientsManagementBackend::Impl final {
 public:
  Impl(GlobalContext* global_context,
       TypedMessageRouter* typed_message_router,
       AdminPolicyGetter* admin_policy_getter)
      : clients_manager_(global_context,
                         typed_message_router,
                         admin_policy_getter) {}

  Impl(const Impl&) = delete;

  ~Impl() { clients_manager_.ShutDown(); }

 private:
  PcscLiteServerClientsManager clients_manager_;
};

PcscLiteServerClientsManagementBackend::PcscLiteServerClientsManagementBackend(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router)
    : impl_(new Impl(global_context,
                     typed_message_router,
                     &admin_policy_getter_)) {}

PcscLiteServerClientsManagementBackend::
    ~PcscLiteServerClientsManagementBackend() {}

}  // namespace google_smart_card
