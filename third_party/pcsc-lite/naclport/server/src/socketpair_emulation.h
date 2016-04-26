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

// As NaCl/PNaCl currently don't implement the sockets for local inter-process
// communication (domain PF_UNIX sockets), their replacement with a limited
// functionality is provided here.
//
// It works only within a single NaCl process. The interface is heavily
// simplified comparing to the original POSIX interface (the family of the
// following functions: accept, bind, close, connect, fcntl, listen, read,
// select, socket, etc.).
//
// When the PNaCl SDK eventually provides a native implementation of the POSIX
// domain sockets (see <http://crbug.com/532095>), this emulation library can be
// dropped.

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SOCKETPAIR_EMULATION_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SOCKETPAIR_EMULATION_H_

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
class SocketpairEmulationManager final {
 public:
  // Creates a singleton instance of this class.
  //
  // Note: This function is not thread-safe!
  static void CreateGlobalInstance();
  // Returns a previously created singleton instance of this class.
  //
  // Note: This function is not thread-safe!
  static SocketpairEmulationManager* GetInstance();

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
  void Write(
      int file_descriptor, const uint8_t* data, int64_t size, bool* is_failure);

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
  bool SelectForReading(
      int file_descriptor, int64_t timeout_milliseconds, bool* is_failure);

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
  bool Read(
      int file_descriptor,
      uint8_t* buffer,
      int64_t* in_out_size,
      bool* is_failure);

 private:
  class Socket;

  SocketpairEmulationManager();

  int GenerateNewFileDescriptor();

  void AddSocket(std::shared_ptr<Socket> socket);

  std::shared_ptr<Socket> FindSocketByFileDescriptor(int file_descriptor) const;

  mutable std::mutex mutex_;
  int next_free_file_descriptor_;
  std::unordered_map<int, const std::shared_ptr<Socket>> socket_map_;
};

namespace socketpair_emulation {

//
// The following group of functions correspond to the methods of the
// SocketpairEmulationManager class.
//
// It is assumed that a global instance of the SocketpairEmulationManager class
// was previously created (see the
// SocketpairEmulationManager::CreateGlobalInstance method).
//

void Create(int* file_descriptor_1, int* file_descriptor_2);

void Close(int file_descriptor, bool* is_failure);

void Write(
    int file_descriptor, const uint8_t* data, int64_t size, bool* is_failure);

void SelectForReading(int file_descriptor, bool* is_failure);
bool SelectForReading(
    int file_descriptor, int64_t timeout_milliseconds, bool* is_failure);

bool Read(
    int file_descriptor,
    uint8_t* buffer,
    int64_t* in_out_size,
    bool* is_failure);

}  // namespace socketpair_emulation

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_SOCKETPAIR_EMULATION_H_
