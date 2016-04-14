/*
 * Copyright (C) 2001-2004
 *  David Corcoran <corcoran@musclecard.com>
 * Copyright (C) 2003-2004
 *  Damien Sauveron <damien.sauveron@labri.fr>
 * Copyright (C) 2002-2010
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 * Copyright (C) 2016 Google Inc.
 *
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This file contains a NaCl port replacement implementation, corresponding to
// the winscard_msg.h PC/SC-Lite internal header file. This implementation
// replaces the winscard_msg.c and winscard_msg_srv.c PC/SC-Lite internal
// implementation files (note that the former one in the original PC/SC-Lite
// library compiles into a client library, and the latter one - into a server
// library; but in this NaCl port there is no such distinction between them).
//
// This file provides a set of functions responsible for communication between
// PC/SC-Lite client library and the PC/SC-Lite server. As in this NaCl port the
// client library is linked together with the server into the same binary, the
// communication channels (which originally were *nix domain sockets) are
// essentially emulated here.

#include <errno.h>

#include <cstdlib>
#include <chrono>

#include <google_smart_card_common/logging/logging.h>

extern "C" {
#include "pcsclite.h"
#include "misc.h"
#include "winscard_msg.h"

// "misc.h" defines a "min" macro which is incompatible with C++.
#undef min
}

#include "server_sockets_manager.h"
#include "socketpair_emulation.h"

// Returns a socket name that should be used for communication between clients
// and daemon.
char* getSocketName() {
  // Return a fake name, as in this PC/SC-Lite NaCl port there are no actual
  // sockets used. However, this function is called in the PC/SC-Lite client
  // library's SCardEstablishContext() implementation, and the socket name is
  // then passed to stat(). So, in order to make it work without patching the
  // source code, an arbitrary existing file path is returned here.
  return const_cast<char*>(FAKE_PCSC_NACL_SOCKET_FILE_NAME);
}

// This function is called by the client library in order to establish a
// communication channel to the daemon.
INTERNAL int ClientSetupSession(uint32_t* pdwClientID) {
  GOOGLE_SMART_CARD_CHECK(pdwClientID);

  // Create an emulated socket pair.

  // One end of the created socket pair is returned as the socket file
  // descriptor for the client library (through the pdwClientID argument).

  int client_socket_file_descriptor;
  int server_socket_file_descriptor;
  google_smart_card::socketpair_emulation::Create(
      &client_socket_file_descriptor, &server_socket_file_descriptor);
  *pdwClientID = static_cast<uint32_t>(client_socket_file_descriptor);

  // Another end of the created socket pair is passed to the daemon main run
  // loop through the PcscLiteServerSocketsManager singleton.

  google_smart_card::PcscLiteServerSocketsManager::GetInstance()->Push(
      server_socket_file_descriptor);

  return 0;
}

// This function is called by the client library in order to close the
// communication channel to the daemon.
INTERNAL int ClientCloseSession(uint32_t dwClientID) {
  bool is_failure = false;
  // Close the client end of the emulated socket pair.
  //
  // Note that the other end of the socket pair, owned by the daemon, is also
  // switched into the "closed" internal state.
  google_smart_card::socketpair_emulation::Close(
      static_cast<int>(dwClientID), &is_failure);
  return is_failure ? -1 : 0;
}

// This is a replacement of the close() standard function, that has to be used
// when dealing with the emulated sockets.
//
// This function is called by the daemon in order to close the communication
// channel to a client.
extern "C" int ServerCloseSession(int fd) {
  bool is_failure = false;
  // Close the daemon end of the emulated socket pair.
  google_smart_card::socketpair_emulation::Close(fd, &is_failure);
  if (is_failure) {
    errno = EBADF;
    return -1;
  }
  return 0;
}

// This function reads data of the specified length from the specified socket
// (which is actually an emulated socket).
//
// This function may be called both by the client library and by the daemon.
INTERNAL LONG MessageReceiveTimeout(
    uint32_t /*command*/,
    void* buffer_void,
    uint64_t buffer_size,
    int32_t filedes,
    long timeOut) {
  GOOGLE_SMART_CARD_CHECK(buffer_void);

  const auto start_time_point = std::chrono::high_resolution_clock::now();
  uint8_t* current_buffer_begin = static_cast<uint8_t*>(buffer_void);
  int64_t left_size = static_cast<int64_t>(buffer_size);
  while (left_size > 0) {
    const auto current_time_point = std::chrono::high_resolution_clock::now();
    const int64_t milliseconds_passed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time_point - start_time_point).count();
    if (milliseconds_passed > timeOut)
      return SCARD_E_TIMEOUT;

    bool is_failure = false;

    if (!google_smart_card::socketpair_emulation::SelectForReading(
             filedes, timeOut - milliseconds_passed, &is_failure)) {
      return is_failure ? SCARD_F_COMM_ERROR : SCARD_E_TIMEOUT;
    }

    int64_t read_size = left_size;
    if (!google_smart_card::socketpair_emulation::Read(
             filedes, current_buffer_begin, &read_size, &is_failure)) {
      return SCARD_F_COMM_ERROR;
    }
    left_size -= read_size;
    current_buffer_begin += read_size;
  }

  return SCARD_S_SUCCESS;
}

// This function transmits the specified data through the specified socket
// (which is actually an emulated socket).
//
// This function may be called both by the client library and by the daemon.
INTERNAL LONG MessageSendWithHeader(
    uint32_t command, uint32_t dwClientID, uint64_t size, void* data_void) {
  struct rxHeader header;
  LONG ret;

  header.command = command;
  header.size = size;
  ret = MessageSend(&header, sizeof(header), dwClientID);

  if (ret == SCARD_S_SUCCESS) {
    if (size > 0)
      ret = MessageSend(data_void, size, dwClientID);
  }

  return ret;
}

// This function transmits the specified data through the specified socket
// (which is actually an emulated socket).
//
// This function may be called both by the client library and by the daemon.
INTERNAL LONG MessageSend(
    void* buffer_void, uint64_t buffer_size, int32_t filedes) {
  bool is_failure = false;
  google_smart_card::socketpair_emulation::Write(
      filedes, static_cast<uint8_t*>(buffer_void), buffer_size, &is_failure);
  return is_failure ? SCARD_F_COMM_ERROR : SCARD_S_SUCCESS;
}

// This function reads data of the specified length from the specified socket
// (which is actually an emulated socket).
//
// This function may be called both by the client library and by the daemon.
INTERNAL LONG MessageReceive(
    void* buffer_void, uint64_t buffer_size, int32_t filedes) {
  GOOGLE_SMART_CARD_CHECK(buffer_void);

  uint8_t* current_buffer_begin = static_cast<uint8_t*>(buffer_void);
  int64_t left_size = static_cast<int64_t>(buffer_size);
  while (left_size > 0) {
    bool is_failure = false;

    google_smart_card::socketpair_emulation::SelectForReading(
        filedes, &is_failure);
    if (is_failure)
      return SCARD_F_COMM_ERROR;

    int64_t read_size = left_size;
    if (!google_smart_card::socketpair_emulation::Read(
             filedes, current_buffer_begin, &read_size, &is_failure)) {
      return SCARD_F_COMM_ERROR;
    }
    left_size -= read_size;
    current_buffer_begin += read_size;
  }

  return SCARD_S_SUCCESS;
}
