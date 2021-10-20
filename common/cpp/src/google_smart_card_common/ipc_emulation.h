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

#include <memory>
#include <mutex>
#include <unordered_map>

namespace google_smart_card {

// This class provides an interface for creating and operating emulated socket
// pairs.
//
// Please note that file descriptors which are provided by this class are not
// real ones: they can only be used with methods of this class.
//
// Also note that the generated file descriptors are not re-used by this class,
// so the emulated sockets may be created only about 2^^31 times (which should
// be enough for most purposes, given that the generation of a new emulated
// socket pair is requested only when a client opens a new connection to the
// server).
class IpcEmulationManager final {
 public:
  // Creates a singleton instance of this class.
  //
  // Note: This function is not thread-safe!
  static void CreateGlobalInstance();
  // Returns a previously created singleton instance of this class.
  //
  // Note: This function is not thread-safe!
  static IpcEmulationManager* GetInstance();

  // Creates a new socket pair, and returns file descriptors corresponding to
  // the both ends.
  void Create(int* file_descriptor_1, int* file_descriptor_2);

  // Closes the file descriptor.
  //
  // If the specified file descriptor is unknown (or already closed), the error
  // is reported through the is_failure argument.
  void Close(int file_descriptor, bool* is_failure);

  // Write the data into the specified end of a previously created socket pair.
  //
  // If the specified file descriptor is unknown (or already closed), the error
  // is reported through the is_failure argument.
  void Write(int file_descriptor,
             const uint8_t* data,
             int64_t size,
             bool* is_failure);

  // Blocks until any data becomes available at the specified end of the socket
  // pair.
  //
  // If the specified file descriptor is unknown (or already closed), the error
  // is reported through the is_failure argument.
  void SelectForReading(int file_descriptor, bool* is_failure);
  // Blocks until any data becomes available at the specified end of the socket
  // pair, or the specified timeout passes.
  //
  // The returned value is true when function returns because the data became
  // available.
  //
  // If the specified file descriptor is unknown (or already closed), the error
  // is reported through the is_failure argument.
  bool SelectForReading(int file_descriptor,
                        int64_t timeout_milliseconds,
                        bool* is_failure);

  // Reads specified number of bytes from the specified end of the socket pair.
  //
  // The returned value is true if at least one byte of data was successfully
  // read.
  //
  // The actual number of read bytes is returned through the input-output
  // in_out_size argument.
  //
  // If the specified file descriptor is unknown (or already closed), the error
  // is reported through the is_failure argument.
  bool Read(int file_descriptor,
            uint8_t* buffer,
            int64_t* in_out_size,
            bool* is_failure);

 private:
  class Socket;

  IpcEmulationManager();
  IpcEmulationManager(const IpcEmulationManager&) = delete;
  IpcEmulationManager& operator=(const IpcEmulationManager&) = delete;
  ~IpcEmulationManager();

  int GenerateNewFileDescriptor();

  void AddSocket(std::shared_ptr<Socket> socket);

  std::shared_ptr<Socket> FindSocketByFileDescriptor(int file_descriptor) const;

  mutable std::mutex mutex_;
  int next_free_file_descriptor_ = 1;
  std::unordered_map<int, const std::shared_ptr<Socket>> socket_map_;
};

namespace ipc_emulation {

//
// The following group of functions correspond to the methods of the
// IpcEmulationManager class.
//
// It is assumed that a global instance of the IpcEmulationManager class was
// previously created (see the `IpcEmulationManager::CreateGlobalInstance()`
// method).
//

void Create(int* file_descriptor_1, int* file_descriptor_2);

void Close(int file_descriptor, bool* is_failure);

void Write(int file_descriptor,
           const uint8_t* data,
           int64_t size,
           bool* is_failure);

void SelectForReading(int file_descriptor, bool* is_failure);
bool SelectForReading(int file_descriptor,
                      int64_t timeout_milliseconds,
                      bool* is_failure);

bool Read(int file_descriptor,
          uint8_t* buffer,
          int64_t* in_out_size,
          bool* is_failure);

}  // namespace ipc_emulation

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_IPC_EMULATION_H_
