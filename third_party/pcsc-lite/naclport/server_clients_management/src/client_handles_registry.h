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
#include <unordered_set>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

// FIXME(emaxx): Write docs.
template <typename T>
class PcscLiteClientHandlesRegistry final {
 public:
  bool Contains(T handle) const {
    const std::unique_lock<std::mutex> lock(mutex_);
    return handles_.count(handle) > 0;
  }

  void Add(T handle) {
    const std::unique_lock<std::mutex> lock(mutex_);
    GOOGLE_SMART_CARD_CHECK(handles_.insert(handle).second);
  }

  void Remove(T handle) {
    const std::unique_lock<std::mutex> lock(mutex_);
    GOOGLE_SMART_CARD_CHECK(handles_.erase(handle));
  }

  std::vector<T> PopAll() {
    const std::unique_lock<std::mutex> lock(mutex_);
    std::vector<T> result(handles_.begin(), handles_.end());
    handles_.clear();
    return result;
  }

 private:
  mutable std::mutex mutex_;
  std::unordered_set<T> handles_;
};

using PcscLiteClientSCardContextsRegistry =
    PcscLiteClientHandlesRegistry<SCARDCONTEXT>;

using PcscLiteClientSCardHandlesRegistry =
    PcscLiteClientHandlesRegistry<SCARDHANDLE>;

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CLIENT_HANDLES_REGISTRY_H_
