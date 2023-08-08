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

#include "client_handles_registry.h"

#include <utility>

#include "common/cpp/src/public/logging/logging.h"

namespace google_smart_card {

PcscLiteClientHandlesRegistry::PcscLiteClientHandlesRegistry() = default;

PcscLiteClientHandlesRegistry::~PcscLiteClientHandlesRegistry() = default;

bool PcscLiteClientHandlesRegistry::ContainsContext(
    SCARDCONTEXT s_card_context) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return context_to_handles_map_.count(s_card_context) > 0;
}

void PcscLiteClientHandlesRegistry::AddContext(SCARDCONTEXT s_card_context) {
  const std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(
      context_to_handles_map_
          .emplace(s_card_context, std::unordered_set<SCARDHANDLE>())
          .second);
}

void PcscLiteClientHandlesRegistry::RemoveContext(SCARDCONTEXT s_card_context) {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto context_to_handles_iter =
      context_to_handles_map_.find(s_card_context);
  GOOGLE_SMART_CARD_CHECK(context_to_handles_iter !=
                          context_to_handles_map_.end());

  for (SCARDHANDLE s_card_handle : context_to_handles_iter->second) {
    const auto handle_to_context_iter =
        handle_to_context_map_.find(s_card_handle);
    GOOGLE_SMART_CARD_CHECK(handle_to_context_iter !=
                            handle_to_context_map_.end());
    GOOGLE_SMART_CARD_CHECK(handle_to_context_iter->second == s_card_context);
    handle_to_context_map_.erase(handle_to_context_iter);
  }
  context_to_handles_map_.erase(context_to_handles_iter);
}

std::vector<SCARDCONTEXT>
PcscLiteClientHandlesRegistry::GetSnapshotOfAllContexts() {
  const std::unique_lock<std::mutex> lock(mutex_);

  std::vector<SCARDCONTEXT> result;
  result.reserve(context_to_handles_map_.size());
  for (const auto& context_to_handles : context_to_handles_map_)
    result.push_back(context_to_handles.first);

  return result;
}

std::vector<SCARDCONTEXT> PcscLiteClientHandlesRegistry::PopAllContexts() {
  const std::unique_lock<std::mutex> lock(mutex_);

  std::vector<SCARDCONTEXT> result;
  result.reserve(context_to_handles_map_.size());
  for (const auto& context_to_handles : context_to_handles_map_)
    result.push_back(context_to_handles.first);

  context_to_handles_map_.clear();
  handle_to_context_map_.clear();

  return result;
}

bool PcscLiteClientHandlesRegistry::ContainsHandle(
    SCARDHANDLE s_card_handle) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  return handle_to_context_map_.count(s_card_handle);
}

SCARDCONTEXT PcscLiteClientHandlesRegistry::FindContextByHandle(
    SCARDHANDLE s_card_handle) const {
  const std::unique_lock<std::mutex> lock(mutex_);

  auto iter = handle_to_context_map_.find(s_card_handle);
  if (iter == handle_to_context_map_.end()) {
    return 0;
  }
  return iter->second;
}

void PcscLiteClientHandlesRegistry::AddHandle(SCARDCONTEXT s_card_context,
                                              SCARDHANDLE s_card_handle) {
  const std::unique_lock<std::mutex> lock(mutex_);

  GOOGLE_SMART_CARD_CHECK(!handle_to_context_map_.count(s_card_handle));

  const auto context_to_handles_iter =
      context_to_handles_map_.find(s_card_context);
  GOOGLE_SMART_CARD_CHECK(context_to_handles_iter !=
                          context_to_handles_map_.end());
  GOOGLE_SMART_CARD_CHECK(
      context_to_handles_iter->second.insert(s_card_handle).second);

  GOOGLE_SMART_CARD_CHECK(
      handle_to_context_map_.emplace(s_card_handle, s_card_context).second);
}

void PcscLiteClientHandlesRegistry::RemoveHandle(SCARDHANDLE s_card_handle) {
  const std::unique_lock<std::mutex> lock(mutex_);

  const auto handle_to_context_iter =
      handle_to_context_map_.find(s_card_handle);
  GOOGLE_SMART_CARD_CHECK(handle_to_context_iter !=
                          handle_to_context_map_.end());
  const SCARDCONTEXT s_card_context = handle_to_context_iter->second;
  handle_to_context_map_.erase(handle_to_context_iter);

  const auto context_to_handles_iter =
      context_to_handles_map_.find(s_card_context);
  GOOGLE_SMART_CARD_CHECK(context_to_handles_iter !=
                          context_to_handles_map_.end());
  GOOGLE_SMART_CARD_CHECK(context_to_handles_iter->second.erase(s_card_handle));
}

}  // namespace google_smart_card
