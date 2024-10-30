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

#include "third_party/pcsc-lite/webport/server/src/server_sockets_manager.h"

#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/optional.h"

namespace google_smart_card {

namespace {

PcscLiteServerSocketsManager* g_pcsc_lite_server_sockets_manager = nullptr;

}  // namespace

// static
void PcscLiteServerSocketsManager::CreateGlobalInstance() {
  GOOGLE_SMART_CARD_CHECK(!g_pcsc_lite_server_sockets_manager);
  g_pcsc_lite_server_sockets_manager = new PcscLiteServerSocketsManager;
}

// static
void PcscLiteServerSocketsManager::DestroyGlobalInstance() {
  // Does nothing if it wasn't created.
  delete g_pcsc_lite_server_sockets_manager;
  g_pcsc_lite_server_sockets_manager = nullptr;
}

// static
PcscLiteServerSocketsManager* PcscLiteServerSocketsManager::GetInstance() {
  GOOGLE_SMART_CARD_CHECK(g_pcsc_lite_server_sockets_manager);
  return g_pcsc_lite_server_sockets_manager;
}

void PcscLiteServerSocketsManager::Push(int server_socket_file_descriptor) {
  const std::unique_lock<std::mutex> lock(mutex_);
  server_socket_file_descriptors_queue_.push(server_socket_file_descriptor);
  condition_.notify_all();
}

optional<int> PcscLiteServerSocketsManager::WaitAndPop() {
  std::unique_lock<std::mutex> lock(mutex_);
  condition_.wait(lock, [this]() {
    return shutting_down_ || !server_socket_file_descriptors_queue_.empty();
  });
  if (shutting_down_)
    return {};
  int result = server_socket_file_descriptors_queue_.front();
  server_socket_file_descriptors_queue_.pop();
  return result;
}

void PcscLiteServerSocketsManager::ShutDown() {
  std::unique_lock<std::mutex> lock(mutex_);
  shutting_down_ = true;
  condition_.notify_all();
}

PcscLiteServerSocketsManager::PcscLiteServerSocketsManager() = default;

PcscLiteServerSocketsManager::~PcscLiteServerSocketsManager() = default;

}  // namespace google_smart_card
