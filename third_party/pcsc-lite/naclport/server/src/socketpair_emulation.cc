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

#include "socketpair_emulation.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <limits>

#include <google_smart_card_common/logging/logging.h>

const char kLoggingPrefix[] = "[emulated domain socket] ";

namespace google_smart_card {

namespace {

SocketpairEmulationManager* g_socketpair_emulation_manager = nullptr;

}  // namespace

class SocketpairEmulationManager::Socket final {
 public:
  explicit Socket(int file_descriptor)
      : file_descriptor_(file_descriptor),
        is_closed_(false) {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "A socket " <<
        file_descriptor << " is created";
  }

  ~Socket() {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "The socket " <<
        file_descriptor() << " is destroyed";
    GOOGLE_SMART_CARD_CHECK(is_closed_);
  }

  int file_descriptor() const {
    return file_descriptor_;
  }

  void SetOtherEnd(std::weak_ptr<Socket> other_end) {
    GOOGLE_SMART_CARD_CHECK(!other_end.expired());
    GOOGLE_SMART_CARD_CHECK(other_end_.expired());
    other_end_ = other_end;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "The socket " <<
        file_descriptor() << " is connected to the emulated domain socket " <<
        other_end_.lock()->file_descriptor();
  }

  void Close() {
    if (SetIsClosed()) {
      const std::shared_ptr<Socket> locked_other_end = other_end_.lock();
      if (locked_other_end)
        locked_other_end->SetIsClosed();
    }
  }

  void Write(const uint8_t* data, int64_t size, bool* is_failure) {
    GOOGLE_SMART_CARD_CHECK(size >= 0);
    if (!size)
      return;
    GOOGLE_SMART_CARD_CHECK(data);
    const std::shared_ptr<Socket> locked_other_end = other_end_.lock();
    if (!locked_other_end) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Writing to the " <<
          "socket " << file_descriptor() << " failed: the other end has " <<
          "already been closed and destroyed";
      return;
    }
    locked_other_end->PushToReadBuffer(data, size, is_failure);
  }

  void SelectForReading(bool* is_failure) {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this]() {
      return is_closed_ || !read_buffer_.empty();
    });
    if (is_closed_) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the " <<
          "socket " << file_descriptor() << " failed: the socket has " <<
          "already been closed";
    }
  }

  bool SelectForReading(int64_t timeout_milliseconds, bool* is_failure) {
    GOOGLE_SMART_CARD_CHECK(timeout_milliseconds >= 0);
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_for(
        lock, std::chrono::milliseconds(timeout_milliseconds), [this]() {
          return is_closed_ || !read_buffer_.empty();
        });
    if (is_closed_) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the " <<
          "socket " << file_descriptor() << " failed: the socket has " <<
          "already been closed";
      return false;
    }
    return !read_buffer_.empty();
  }

  bool Read(uint8_t* buffer, int64_t* in_out_size, bool* is_failure) {
    GOOGLE_SMART_CARD_CHECK(buffer);
    GOOGLE_SMART_CARD_CHECK(*in_out_size > 0);
    std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Reading from the " <<
          "socket " << file_descriptor() << " failed: the socket has " <<
          "already been closed";
      return false;
    }
    if (read_buffer_.empty())
      return false;
    *in_out_size = std::min(
        *in_out_size, static_cast<int64_t>(read_buffer_.size()));
    std::copy(
        read_buffer_.begin(), read_buffer_.begin() + *in_out_size, buffer);
    read_buffer_.erase(
        read_buffer_.begin(), read_buffer_.begin() + *in_out_size);
    return true;
  }

 private:
  bool SetIsClosed() {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_)
      return false;
    is_closed_ = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "The socket " <<
        file_descriptor() << " is closed";
    condition_.notify_all();
    return true;
  }

  void PushToReadBuffer(const uint8_t* data, int64_t size, bool* is_failure) {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Failed to push data " <<
          "into the socket " << file_descriptor() << ": the socket is " <<
          "already closed";
      return;
    }
    read_buffer_.insert(read_buffer_.end(), data, data + size);
    condition_.notify_all();
  }

  const int file_descriptor_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  bool is_closed_;
  std::weak_ptr<Socket> other_end_;
  std::deque<uint8_t> read_buffer_;
};

void SocketpairEmulationManager::CreateGlobalInstance() {
  GOOGLE_SMART_CARD_CHECK(!g_socketpair_emulation_manager);
  g_socketpair_emulation_manager = new SocketpairEmulationManager;
}

SocketpairEmulationManager* SocketpairEmulationManager::GetInstance() {
  GOOGLE_SMART_CARD_CHECK(g_socketpair_emulation_manager);
  return g_socketpair_emulation_manager;
}

void SocketpairEmulationManager::Create(
    int* file_descriptor_1, int* file_descriptor_2) {
  GOOGLE_SMART_CARD_CHECK(file_descriptor_1);
  GOOGLE_SMART_CARD_CHECK(file_descriptor_2);
  *file_descriptor_1 = GenerateNewFileDescriptor();
  *file_descriptor_2 = GenerateNewFileDescriptor();
  std::shared_ptr<Socket> socket_1(new Socket(*file_descriptor_1));
  std::shared_ptr<Socket> socket_2(new Socket(*file_descriptor_2));
  socket_1->SetOtherEnd(socket_2);
  socket_2->SetOtherEnd(socket_1);
  AddSocket(std::move(socket_1));
  AddSocket(std::move(socket_2));
}

void SocketpairEmulationManager::Close(int file_descriptor, bool* is_failure) {
  const std::unique_lock<std::mutex> lock(mutex_);
  const auto socket_map_iter = socket_map_.find(file_descriptor);
  if (socket_map_iter == socket_map_.end()) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Closing of the socket " <<
        file_descriptor << " failed: the requested socket is " << "already " <<
        "destroyed or never existed";
  }
  socket_map_iter->second->Close();
  socket_map_.erase(socket_map_iter);
}

void SocketpairEmulationManager::Write(
    int file_descriptor, const uint8_t* data, int64_t size, bool* is_failure) {
  const std::shared_ptr<Socket> socket = FindSocketByFileDescriptor(
      file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Writing to the socket " <<
        file_descriptor << " failed: the requested socket is already " <<
        "destroyed or never existed";
    return;
  }
  socket->Write(data, size, is_failure);
}

void SocketpairEmulationManager::SelectForReading(
    int file_descriptor, bool* is_failure) {
  const std::shared_ptr<Socket> socket = FindSocketByFileDescriptor(
      file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the " <<
        "socket " << file_descriptor << " failed: the requested socket is " <<
        "already destroyed or never existed";
    return;
  }
  socket->SelectForReading(is_failure);
}

bool SocketpairEmulationManager::SelectForReading(
    int file_descriptor, int64_t timeout_milliseconds, bool* is_failure) {
  const std::shared_ptr<Socket> socket = FindSocketByFileDescriptor(
      file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the " <<
        "socket " << file_descriptor << " failed: the requested socket is " <<
        "already destroyed or never existed";
    return false;
  }
  return socket->SelectForReading(timeout_milliseconds, is_failure);
}

bool SocketpairEmulationManager::Read(
    int file_descriptor,
    uint8_t* buffer,
    int64_t* in_out_size,
    bool* is_failure) {
  const std::shared_ptr<Socket> socket = FindSocketByFileDescriptor(
      file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Reading from the " <<
        "socket " << file_descriptor << " failed: the requested socket is " <<
        "already destroyed or never existed";
    return false;
  }
  if (!socket->Read(buffer, in_out_size, is_failure))
    return false;
  GOOGLE_SMART_CARD_CHECK(*in_out_size > 0);
  return true;
}

SocketpairEmulationManager::SocketpairEmulationManager()
    : next_free_file_descriptor_(1) {}

int SocketpairEmulationManager::GenerateNewFileDescriptor() {
  const std::unique_lock<std::mutex> lock(mutex_);
  const int file_descriptor = next_free_file_descriptor_;
  // FIXME(emaxx): Implement keeping a set of unused file descriptors instead
  // of using the simple counter (which will exhaust at some point - though not
  // very realistically, because a new emulated file descriptor is generated
  // only when a client opens a new connection to the server).
  GOOGLE_SMART_CARD_CHECK(file_descriptor < std::numeric_limits<int>::max());
  ++next_free_file_descriptor_;
  return file_descriptor;
}

void SocketpairEmulationManager::AddSocket(std::shared_ptr<Socket> socket) {
  GOOGLE_SMART_CARD_CHECK(socket);
  GOOGLE_SMART_CARD_CHECK(socket.unique());
  const std::unique_lock<std::mutex> lock(mutex_);
  GOOGLE_SMART_CARD_CHECK(socket_map_.emplace(
      socket->file_descriptor(), std::move(socket)).second);
}

std::shared_ptr<SocketpairEmulationManager::Socket>
SocketpairEmulationManager::FindSocketByFileDescriptor(
    int file_descriptor) const {
  const std::unique_lock<std::mutex> lock(mutex_);
  const auto socket_map_iter = socket_map_.find(file_descriptor);
  if (socket_map_iter == socket_map_.end())
    return {};
  return socket_map_iter->second;
}

namespace socketpair_emulation {

void Create(int* file_descriptor_1, int* file_descriptor_2) {
  SocketpairEmulationManager::GetInstance()->Create(
      file_descriptor_1, file_descriptor_2);
}

void Close(int file_descriptor, bool* is_failure) {
  SocketpairEmulationManager::GetInstance()->Close(file_descriptor, is_failure);
}

void Write(
    int file_descriptor, const uint8_t* data, int64_t size, bool* is_failure) {
  SocketpairEmulationManager::GetInstance()->Write(
      file_descriptor, data, size, is_failure);
}

void SelectForReading(int file_descriptor, bool* is_failure) {
  SocketpairEmulationManager::GetInstance()->SelectForReading(
      file_descriptor, is_failure);
}

bool SelectForReading(
    int file_descriptor, int64_t timeout_milliseconds, bool* is_failure) {
  return SocketpairEmulationManager::GetInstance()->SelectForReading(
      file_descriptor, timeout_milliseconds, is_failure);
}

bool Read(
    int file_descriptor,
    uint8_t* buffer,
    int64_t* in_out_size,
    bool* is_failure) {
  return SocketpairEmulationManager::GetInstance()->Read(
      file_descriptor, buffer, in_out_size, is_failure);
}

}  // namespace socketpair_emulation

}  // namespace google_smart_card
