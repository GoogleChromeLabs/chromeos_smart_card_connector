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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CLIENT_HANDLES_REGISTRY_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CLIENT_HANDLES_REGISTRY_H_

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <pcsclite.h>

namespace google_smart_card {

// This class is a thread-safe container for PC/SC-Lite contexts (see the
// PC/SC-Lite SCARDCONTEXT type) and handles (see the PC/SC-Lite SCARDHANDLE
// type).
//
// The class provides an interface for storing and querying a number of contexts
// and handles. The interface also allows to store and check association between
// a handle and a context.
class PcscLiteClientHandlesRegistry final {
 public:
  PcscLiteClientHandlesRegistry();
  PcscLiteClientHandlesRegistry(const PcscLiteClientHandlesRegistry&) = delete;
  PcscLiteClientHandlesRegistry& operator=(
      const PcscLiteClientHandlesRegistry&) = delete;
  ~PcscLiteClientHandlesRegistry();

  bool ContainsContext(SCARDCONTEXT s_card_context) const;
  // Adds the context to the data structure. CHECKs that it wasn't already
  // present.
  void AddContext(SCARDCONTEXT s_card_context);
  // Removes the context from the data structure. CHECKs that it was present.
  void RemoveContext(SCARDCONTEXT s_card_context);
  // Returns all contexts currently stored in the data structure.
  std::vector<SCARDCONTEXT> GetSnapshotOfAllContexts();
  // Returns all contexts currently stored in the data structure and clears it.
  std::vector<SCARDCONTEXT> PopAllContexts();

  bool ContainsHandle(SCARDHANDLE s_card_handle) const;
  // Adds the handle to the data structure. CHECKs that it wasn't already
  // present.
  void AddHandle(SCARDCONTEXT s_card_context, SCARDHANDLE s_card_handle);
  // Removes the handle from the data structure. CHECKs that it was present.
  void RemoveHandle(SCARDHANDLE s_card_handle);

 private:
  using ContextToHandlesMap =
      std::unordered_map<SCARDCONTEXT, std::unordered_set<SCARDHANDLE>>;
  using HandleToContextMap = std::unordered_map<SCARDHANDLE, SCARDCONTEXT>;

  mutable std::mutex mutex_;
  ContextToHandlesMap context_to_handles_map_;
  HandleToContextMap handle_to_context_map_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CLIENT_HANDLES_REGISTRY_H_
