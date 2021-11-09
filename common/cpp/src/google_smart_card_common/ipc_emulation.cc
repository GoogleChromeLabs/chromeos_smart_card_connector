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

#include <google_smart_card_common/cpp_attributes.h>
#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/optional.h>

namespace google_smart_card {

namespace {

constexpr char kLoggingPrefix[] = "[emulated IPC] ";

IpcEmulation* g_ipc_emulation = nullptr;

}  // namespace

class IpcEmulation::InMemoryFile final {
 public:
  explicit InMemoryFile(int file_descriptor)
      : file_descriptor_(file_descriptor) {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "An in-memory file "
                                << file_descriptor << " was created";
  }

  InMemoryFile(const InMemoryFile&) = delete;
  InMemoryFile& operator=(const InMemoryFile&) = delete;

  ~InMemoryFile() {
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "The in-memory file "
                                << file_descriptor() << " was destroyed";
    GOOGLE_SMART_CARD_CHECK(is_closed_);
  }

  int file_descriptor() const { return file_descriptor_; }

  void SetOtherEnd(std::weak_ptr<InMemoryFile> other_end) {
    GOOGLE_SMART_CARD_CHECK(!other_end.expired());
    GOOGLE_SMART_CARD_CHECK(other_end_.expired());
    other_end_ = other_end;
    GOOGLE_SMART_CARD_LOG_DEBUG
        << kLoggingPrefix << "The in-memory file " << file_descriptor()
        << " connected to the in-memory file "
        << other_end_.lock()->file_descriptor();
  }

  void Close() {
    if (SetIsClosed()) {
      const std::shared_ptr<InMemoryFile> locked_other_end = other_end_.lock();
      if (locked_other_end)
        locked_other_end->SetIsClosed();
    }
  }

  bool Write(const uint8_t* data, int64_t size) {
    GOOGLE_SMART_CARD_CHECK(size >= 0);
    if (!size)
      return true;
    GOOGLE_SMART_CARD_CHECK(data);
    const std::shared_ptr<InMemoryFile> locked_other_end = other_end_.lock();
    if (!locked_other_end)
      return false;
    return locked_other_end->PushToReadBuffer(data, size);
  }

  IpcEmulation::WaitResult WaitUntilCanBeRead(
      optional<int64_t> timeout_milliseconds)
      GOOGLE_SMART_CARD_WARN_UNUSED_RESULT {
    GOOGLE_SMART_CARD_CHECK(!timeout_milliseconds ||
                            *timeout_milliseconds >= 0);
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_for(
        lock,
        timeout_milliseconds ? std::chrono::milliseconds(*timeout_milliseconds)
                             : std::chrono::milliseconds::max(),
        [this]() { return is_closed_ || !read_buffer_.empty(); });
    if (is_closed_)
      return IpcEmulation::WaitResult::kNoSuchFile;
    if (read_buffer_.empty())
      return IpcEmulation::WaitResult::kTimeout;
    return IpcEmulation::WaitResult::kSuccess;
  }

  IpcEmulation::ReadResult Read(uint8_t* buffer, int64_t* in_out_size)
      GOOGLE_SMART_CARD_WARN_UNUSED_RESULT {
    GOOGLE_SMART_CARD_CHECK(buffer);
    GOOGLE_SMART_CARD_CHECK(*in_out_size > 0);
    std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_)
      return IpcEmulation::ReadResult::kNoSuchFile;
    if (read_buffer_.empty())
      return IpcEmulation::ReadResult::kNoData;
    *in_out_size =
        std::min(*in_out_size, static_cast<int64_t>(read_buffer_.size()));
    std::copy(read_buffer_.begin(), read_buffer_.begin() + *in_out_size,
              buffer);
    read_buffer_.erase(read_buffer_.begin(),
                       read_buffer_.begin() + *in_out_size);
    return IpcEmulation::ReadResult::kSuccess;
  }

 private:
  bool SetIsClosed() {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_)
      return false;
    is_closed_ = true;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "The in-memory file "
                                << file_descriptor() << " was closed";
    condition_.notify_all();
    return true;
  }

  bool PushToReadBuffer(const uint8_t* data,
                        int64_t size) GOOGLE_SMART_CARD_WARN_UNUSED_RESULT {
    const std::unique_lock<std::mutex> lock(mutex_);
    if (is_closed_)
      return false;
    read_buffer_.insert(read_buffer_.end(), data, data + size);
    condition_.notify_all();
    return true;
  }

  const int file_descriptor_;
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  bool is_closed_ = false;
  std::weak_ptr<InMemoryFile> other_end_;
  std::deque<uint8_t> read_buffer_;
};

// static
void IpcEmulation::CreateGlobalInstance() {
  GOOGLE_SMART_CARD_CHECK(!g_ipc_emulation);
  g_ipc_emulation = new IpcEmulation;
}

// static
IpcEmulation* IpcEmulation::GetInstance() {
  GOOGLE_SMART_CARD_CHECK(g_ipc_emulation);
  return g_ipc_emulation;
}

void IpcEmulation::CreateInMemoryFilePair(int* file_descriptor_1,
                                          int* file_descriptor_2) {
  GOOGLE_SMART_CARD_CHECK(file_descriptor_1);
  GOOGLE_SMART_CARD_CHECK(file_descriptor_2);
  *file_descriptor_1 = GenerateNewFileDescriptor();
  *file_descriptor_2 = GenerateNewFileDescriptor();
  std::shared_ptr<InMemoryFile> file_1(new InMemoryFile(*file_descriptor_1));
  std::shared_ptr<InMemoryFile> file_2(new InMemoryFile(*file_descriptor_2));
  file_1->SetOtherEnd(file_2);
  file_2->SetOtherEnd(file_1);
  AddFile(std::move(file_1));
  AddFile(std::move(file_2));
}

bool IpcEmulation::CloseInMemoryFile(int file_descriptor) {
  const std::unique_lock<std::mutex> lock(mutex_);
  const auto iter = file_descriptor_to_file_map_.find(file_descriptor);
  if (iter == file_descriptor_to_file_map_.end())
    return false;
  iter->second->Close();
  file_descriptor_to_file_map_.erase(iter);
  // Return true regardless of the `is_closed_` flag of the closed file: we're
  // removing the file from the map after closing it, and thanks to the mutex
  // here there's no other thread that could close it or its other endpoint
  // concurrently.
  return true;
}

bool IpcEmulation::WriteToInMemoryFile(int file_descriptor,
                                       const uint8_t* data,
                                       int64_t size) {
  const std::shared_ptr<InMemoryFile> file =
      FindFileByDescriptor(file_descriptor);
  if (!file)
    return false;
  return file->Write(data, size);
}

IpcEmulation::WaitResult IpcEmulation::WaitForInMemoryFileCanBeRead(
    int file_descriptor,
    optional<int64_t> timeout_milliseconds) {
  const std::shared_ptr<InMemoryFile> file =
      FindFileByDescriptor(file_descriptor);
  if (!file)
    return WaitResult::kNoSuchFile;
  return file->WaitUntilCanBeRead(timeout_milliseconds);
}

IpcEmulation::ReadResult IpcEmulation::ReadFromInMemoryFile(
    int file_descriptor,
    uint8_t* buffer,
    int64_t* in_out_size) {
  const std::shared_ptr<InMemoryFile> file =
      FindFileByDescriptor(file_descriptor);
  if (!file)
    return ReadResult::kNoSuchFile;
  ReadResult read_result = file->Read(buffer, in_out_size);
  GOOGLE_SMART_CARD_CHECK(*in_out_size > 0);
  return read_result;
}

IpcEmulation::IpcEmulation() = default;

IpcEmulation::~IpcEmulation() = default;

int IpcEmulation::GenerateNewFileDescriptor() {
  const std::unique_lock<std::mutex> lock(mutex_);
  const int file_descriptor = next_free_file_descriptor_;
  GOOGLE_SMART_CARD_CHECK(file_descriptor < std::numeric_limits<int>::max());
  ++next_free_file_descriptor_;
  return file_descriptor;
}

void IpcEmulation::AddFile(std::shared_ptr<InMemoryFile> file) {
  GOOGLE_SMART_CARD_CHECK(file);
  GOOGLE_SMART_CARD_CHECK(file.unique());
  const std::unique_lock<std::mutex> lock(mutex_);
  const int file_descriptor = file->file_descriptor();
  GOOGLE_SMART_CARD_CHECK(
      file_descriptor_to_file_map_.emplace(file_descriptor, std::move(file))
          .second);
}

std::shared_ptr<IpcEmulation::InMemoryFile> IpcEmulation::FindFileByDescriptor(
    int file_descriptor) const {
  const std::unique_lock<std::mutex> lock(mutex_);
  const auto iter = file_descriptor_to_file_map_.find(file_descriptor);
  if (iter == file_descriptor_to_file_map_.end())
    return {};
  return iter->second;
}

}  // namespace google_smart_card
