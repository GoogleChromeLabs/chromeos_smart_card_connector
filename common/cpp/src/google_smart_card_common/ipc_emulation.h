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

// This file provides polyfills for POSIX inter-process communication
// primitives. As web packaging technologies (WebAssembly, and previously NaCl)
// don't support multiprocess execution, our polyfills are in-process simulation
// of these primitives. The polyfills are also severely simplified, with the
// main objective to address our use cases in this project.

#ifndef GOOGLE_SMART_CARD_COMMON_IPC_EMULATION_H_
#define GOOGLE_SMART_CARD_COMMON_IPC_EMULATION_H_

#include <stdint.h>
#include <sys/types.h>

#include <memory>
#include <mutex>
#include <unordered_map>

#include <google_smart_card_common/cpp_attributes.h>
#include <google_smart_card_common/optional.h>

namespace google_smart_card {

// This class provides an emulated replacement for some IPC (inter-process
// communication) primitives.
//
// Please note that file descriptors which are provided by this class are not
// real ones: they can only be used with methods of this class.
//
// Also note that the generated file descriptors are not re-used by this class,
// so the in-memory files may be created only about 2^^31 times (which should
// be enough for most purposes, given that the generation of a new emulated
// socket pair is requested only when a client opens a new connection to the
// server).
class IpcEmulation final {
 public:
  enum class WaitResult {
    kSuccess,
    kNoSuchFile,
    kTimeout,
  };
  enum class ReadResult {
    kSuccess,
    kNoSuchFile,
    kNoData,
  };

  // Creates a singleton instance of this class.
  //
  // Note: This function is not thread-safe!
  static void CreateGlobalInstance();
  // Destroys the singleton instance created by `CreateGlobalInstance()`.
  // Non-thread-safe.
  static void DestroyGlobalInstanceForTesting();
  // Returns a previously created singleton instance of this class.
  //
  // Note: This function is not thread-safe!
  static IpcEmulation* GetInstance();

  // Creates a pair of in-memory files that are linked with each other (data
  // written into one can be read from another).
  void CreateInMemoryFilePair(int* file_descriptor_1,
                              int* file_descriptor_2,
                              bool reads_should_block);

  // Closes the file descriptor.
  //
  // Returns false if the specified file descriptor is unknown or already
  // closed.
  bool CloseInMemoryFile(int file_descriptor)
      GOOGLE_SMART_CARD_WARN_UNUSED_RESULT;

  // Write the data into the specified file descriptor. This makes the data
  // available on the other end of the file descriptor pair.
  //
  // Returns false if the specified file descriptor is unknown or already
  // closed.
  bool WriteToInMemoryFile(int file_descriptor,
                           const uint8_t* data,
                           int64_t size) GOOGLE_SMART_CARD_WARN_UNUSED_RESULT;

  // Blocks until any data becomes available for reading from the given file, or
  // the specified timeout passes, or an error occurs.
  WaitResult WaitForInMemoryFileCanBeRead(
      int file_descriptor,
      optional<int64_t> timeout_milliseconds)
      GOOGLE_SMART_CARD_WARN_UNUSED_RESULT;

  // Reads specified number of bytes from the specified end of the socket pair.
  // Does *not* block until the data becomes available.
  //
  // The returned result is successful if at least one byte of data was read.
  //
  // The actual number of read bytes is returned through the input-output
  // `in_out_size` argument.
  ReadResult ReadFromInMemoryFile(int file_descriptor,
                                  uint8_t* buffer,
                                  int64_t* in_out_size)
      GOOGLE_SMART_CARD_WARN_UNUSED_RESULT;

 private:
  class InMemoryFile;

  IpcEmulation();
  IpcEmulation(const IpcEmulation&) = delete;
  IpcEmulation& operator=(const IpcEmulation&) = delete;
  ~IpcEmulation();

  int GenerateNewFileDescriptor();

  void AddFile(std::shared_ptr<InMemoryFile> file);

  std::shared_ptr<InMemoryFile> FindFileByDescriptor(int file_descriptor) const;

  mutable std::mutex mutex_;
  int next_free_file_descriptor_ = 1;
  std::unordered_map<int, const std::shared_ptr<InMemoryFile>>
      file_descriptor_to_file_map_;
};

}  // namespace google_smart_card

// Global functions that are wrappers around the `IpcEmulation` class. They are
// needed when we want to be called from C code; C++ code should use the class
// directly instead.
extern "C" {

// Fake implementation of pipe().
//
// It creates a pair of fake file descriptors using the `IpcEmulation` class.
//
// The background is that the standard library implementation of `pipe()` under
// Emscripten has poor semantics: it always creates a nonblocking pipe, despite
// that the `O_NONBLOCK` flag is not passed.
int GoogleSmartCardIpcEmulationPipe(int pipefd[2]);

// Fake implementation of `write()`.
//
// It only supports the fake file descriptors that are created via helpers in
// this file.
ssize_t GoogleSmartCardIpcEmulationWrite(int fd, const void* buf, size_t count);

// Fake implementation of `read()`.
//
// It only supports the fake file descriptors that are created via helpers in
// this file.
ssize_t GoogleSmartCardIpcEmulationRead(int fd, void* buf, size_t count);

// Fake implementation of `close()`.
//
// It only supports the fake file descriptors that are created via helpers in
// this file.
int GoogleSmartCardIpcEmulationClose(int fd);

}  // extern "C"

#endif  // GOOGLE_SMART_CARD_COMMON_IPC_EMULATION_H_
