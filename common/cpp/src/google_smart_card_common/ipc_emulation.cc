// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <google_smart_card_common/ipc_emulation.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <limits>

#include <google_smart_card_common/logging/logging.h>

namespace google_smart_card {

namespace {

constexpr char kLoggingPrefix[] = "[emulated domain socket] ";

IpcEmulationManager* g_ipc_emulation_manager = nullptr;

}  // namespace

class IpcEmulationManager::Socket final {
 public:
  explicit Socket(int file_descriptor) : file_descriptor_(file_descriptor) {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "A socket "
                                << file_descriptor << " is created";
  }

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  ~Socket() {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "The socket "
                                << file_descriptor() << " is destroyed";
    GOOGLE_SMART_CARD_CHECK(is_closed_);
  }

  int file_descriptor() const { return file_descriptor_; }

  void SetOtherEnd(std::weak_ptr<Socket> other_end) {
    GOOGLE_SMART_CARD_CHECK(!other_end.expired());
    GOOGLE_SMART_CARD_CHECK(other_end_.expired());
    other_end_ = other_end;
    GOOGLE_SMART_CARD_LOG_DEBUG
        << kLoggingPrefix << "The socket " << file_descriptor()
        << " is connected to the emulated domain socket "
        << other_end_.lock()->file_descriptor();
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
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Writing to the "
                                  << "socket " << file_descriptor()
                                  << " failed: the other end has "
                                  << "already been closed and destroyed";
      return;
    }
    locked_other_end->PushToReadBuffer(data, size, is_failure);
  }

  void SelectForReading(bool* is_failure) {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock,
                    [this]() { return is_closed_ || !read_buffer_.empty(); });
    if (is_closed_) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the "
                                  << "socket " << file_descriptor()
                                  << " failed: the socket has "
                                  << "already been closed";
    }
  }

  bool SelectForReading(int64_t timeout_milliseconds, bool* is_failure) {
    GOOGLE_SMART_CARD_CHECK(timeout_milliseconds >= 0);
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_for(
        lock, std::chrono::milliseconds(timeout_milliseconds),
        [this]() { return is_closed_ || !read_buffer_.empty(); });
    if (is_closed_) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the "
                                  << "socket " << file_descriptor()
                                  << " failed: the socket has "
                                  << "already been closed";
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
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Reading from the "
                                  << "socket " << file_descriptor()
                                  << " failed: the socket has "
                                  << "already been closed";
      return false;
    }
    if (read_buffer_.empty())
      return false;
    *in_out_size =
        std::min(*in_out_size, static_cast<int64_t>(read_buffer_.size()));
    std::copy(read_buffer_.begin(), read_buffer_.begin() + *in_out_size,
              buffer);
    read_buffer_.erase(read_buffer_.begin(),
                       read_buffer_.begin() + *in_out_size);
    return true;
  }

 private:
  bool SetIsClosed() {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_)
      return false;
    is_closed_ = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "The socket "
                                << file_descriptor() << " is closed";
    condition_.notify_all();
    return true;
  }

  void PushToReadBuffer(const uint8_t* data, int64_t size, bool* is_failure) {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_) {
      *is_failure = true;
      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Failed to push data "
                                  << "into the socket " << file_descriptor()
                                  << ": the socket is "
                                  << "already closed";
      return;
    }
    read_buffer_.insert(read_buffer_.end(), data, data + size);
    condition_.notify_all();
  }

  const int file_descriptor_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  bool is_closed_ = false;
  std::weak_ptr<Socket> other_end_;
  std::deque<uint8_t> read_buffer_;
};

// static
void IpcEmulationManager::CreateGlobalInstance() {
  GOOGLE_SMART_CARD_CHECK(!g_ipc_emulation_manager);
  g_ipc_emulation_manager = new IpcEmulationManager;
}

// static
IpcEmulationManager* IpcEmulationManager::GetInstance() {
  GOOGLE_SMART_CARD_CHECK(g_ipc_emulation_manager);
  return g_ipc_emulation_manager;
}

void IpcEmulationManager::Create(int* file_descriptor_1,
                                 int* file_descriptor_2) {
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

void IpcEmulationManager::Close(int file_descriptor, bool* is_failure) {
  const std::unique_lock<std::mutex> lock(mutex_);
  const auto socket_map_iter = socket_map_.find(file_descriptor);
  if (socket_map_iter == socket_map_.end()) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Closing of the socket "
                                << file_descriptor
                                << " failed: the requested socket is "
                                << "already "
                                << "destroyed or never existed";
  }
  socket_map_iter->second->Close();
  socket_map_.erase(socket_map_iter);
}

void IpcEmulationManager::Write(int file_descriptor,
                                const uint8_t* data,
                                int64_t size,
                                bool* is_failure) {
  const std::shared_ptr<Socket> socket =
      FindSocketByFileDescriptor(file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Writing to the socket "
                                << file_descriptor
                                << " failed: the requested socket is already "
                                << "destroyed or never existed";
    return;
  }
  socket->Write(data, size, is_failure);
}

void IpcEmulationManager::SelectForReading(int file_descriptor,
                                           bool* is_failure) {
  const std::shared_ptr<Socket> socket =
      FindSocketByFileDescriptor(file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the "
                                << "socket " << file_descriptor
                                << " failed: the requested socket is "
                                << "already destroyed or never existed";
    return;
  }
  socket->SelectForReading(is_failure);
}

bool IpcEmulationManager::SelectForReading(int file_descriptor,
                                           int64_t timeout_milliseconds,
                                           bool* is_failure) {
  const std::shared_ptr<Socket> socket =
      FindSocketByFileDescriptor(file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Selecting from the "
                                << "socket " << file_descriptor
                                << " failed: the requested socket is "
                                << "already destroyed or never existed";
    return false;
  }
  return socket->SelectForReading(timeout_milliseconds, is_failure);
}

bool IpcEmulationManager::Read(int file_descriptor,
                               uint8_t* buffer,
                               int64_t* in_out_size,
                               bool* is_failure) {
  const std::shared_ptr<Socket> socket =
      FindSocketByFileDescriptor(file_descriptor);
  if (!socket) {
    *is_failure = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "Reading from the "
                                << "socket " << file_descriptor
                                << " failed: the requested socket is "
                                << "already destroyed or never existed";
    return false;
  }
  if (!socket->Read(buffer, in_out_size, is_failure))
    return false;
  GOOGLE_SMART_CARD_CHECK(*in_out_size > 0);
  return true;
}

IpcEmulationManager::IpcEmulationManager() = default;

IpcEmulationManager::~IpcEmulationManager() = default;

int IpcEmulationManager::GenerateNewFileDescriptor() {
  const std::unique_lock<std::mutex> lock(mutex_);
  const int file_descriptor = next_free_file_descriptor_;
  GOOGLE_SMART_CARD_CHECK(file_descriptor < std::numeric_limits<int>::max());
  ++next_free_file_descriptor_;
  return file_descriptor;
}

void IpcEmulationManager::AddSocket(std::shared_ptr<Socket> socket) {
  GOOGLE_SMART_CARD_CHECK(socket);
  GOOGLE_SMART_CARD_CHECK(socket.unique());
  const std::unique_lock<std::mutex> lock(mutex_);
  const int file_descriptor = socket->file_descriptor();
  GOOGLE_SMART_CARD_CHECK(
      socket_map_.emplace(file_descriptor, std::move(socket)).second);
}

std::shared_ptr<IpcEmulationManager::Socket>
IpcEmulationManager::FindSocketByFileDescriptor(int file_descriptor) const {
  const std::unique_lock<std::mutex> lock(mutex_);
  const auto socket_map_iter = socket_map_.find(file_descriptor);
  if (socket_map_iter == socket_map_.end())
    return {};
  return socket_map_iter->second;
}

namespace ipc_emulation {

void Create(int* file_descriptor_1, int* file_descriptor_2) {
  IpcEmulationManager::GetInstance()->Create(file_descriptor_1,
                                             file_descriptor_2);
}

void Close(int file_descriptor, bool* is_failure) {
  IpcEmulationManager::GetInstance()->Close(file_descriptor, is_failure);
}

void Write(int file_descriptor,
           const uint8_t* data,
           int64_t size,
           bool* is_failure) {
  IpcEmulationManager::GetInstance()->Write(file_descriptor, data, size,
                                            is_failure);
}

void SelectForReading(int file_descriptor, bool* is_failure) {
  IpcEmulationManager::GetInstance()->SelectForReading(file_descriptor,
                                                       is_failure);
}

bool SelectForReading(int file_descriptor,
                      int64_t timeout_milliseconds,
                      bool* is_failure) {
  return IpcEmulationManager::GetInstance()->SelectForReading(
      file_descriptor, timeout_milliseconds, is_failure);
}

bool Read(int file_descriptor,
          uint8_t* buffer,
          int64_t* in_out_size,
          bool* is_failure) {
  return IpcEmulationManager::GetInstance()->Read(file_descriptor, buffer,
                                                  in_out_size, is_failure);
}

}  // namespace ipc_emulation

}  // namespace google_smart_card
