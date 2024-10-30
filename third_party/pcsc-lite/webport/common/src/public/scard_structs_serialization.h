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

// This file contains structure and function definitions that help to perform
// (de)serialization of PC/SC-Lite API structures from/to `Value`s.
//
// The main reason for why this file defines separate intermediate structures is
// memory management: the C structures from the PC/SC-Lite API use raw pointers
// (some of which are owned and some unowned), have separate fields for storing
// the size of the referenced pointer, etc. Instead of writing custom
// (de)serialization code, we convert those C structures into C++ intermediate
// structures that have clear memory management.

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_SRC_PUBLIC_SCARD_STRUCTS_SERIALIZATION_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_SRC_PUBLIC_SCARD_STRUCTS_SERIALIZATION_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

namespace google_smart_card {

// Replacement of the SCARD_READERSTATE PC/SC-Lite structure that should be used
// when parsing the inbound data (i.e. PC/SC-Lite API arguments).
//
// This replacement structure solves, among others, the following problems:
// * Distinction between output-only fields and the other fields (see the
//   OutboundSCardReaderState structure below);
// * Storing and owning of the reader_name object of type std::string.
struct InboundSCardReaderState {
  InboundSCardReaderState() = default;

  InboundSCardReaderState(const std::string& reader_name,
                          std::optional<uintptr_t> user_data,
                          DWORD current_state)
      : reader_name(reader_name),
        user_data(user_data),
        current_state(current_state) {}

  static InboundSCardReaderState FromSCardReaderState(
      const SCARD_READERSTATE& value);

  std::string reader_name;
  std::optional<uintptr_t> user_data;
  DWORD current_state;
};

// Replacement of the SCARD_READERSTATE PC/SC-Lite structure that should be used
// when parsing the outbound data (i.e. values returned by PC/SC-Lite API
// calls).
//
// This replacement structure solves, among others, the following problems:
// * Distinction between output-only fields and the other fields (see the
//   InboundSCardReaderState structure above);
// * Storing and owning of the reader_name object of type std::string.
struct OutboundSCardReaderState {
  OutboundSCardReaderState() = default;

  OutboundSCardReaderState(const std::string& reader_name,
                           std::optional<uintptr_t> user_data,
                           DWORD current_state,
                           DWORD event_state,
                           const std::vector<uint8_t>& atr)
      : reader_name(reader_name),
        user_data(user_data),
        current_state(current_state),
        event_state(event_state),
        atr(atr) {}

  static OutboundSCardReaderState FromSCardReaderState(
      const SCARD_READERSTATE& value);

  std::string reader_name;
  std::optional<uintptr_t> user_data;
  DWORD current_state;
  DWORD event_state;
  std::vector<uint8_t> atr;
};

// Replacement of the SCARD_IO_REQUEST PC/SC-Lite structure.
struct SCardIoRequest {
  SCardIoRequest() = default;

  explicit SCardIoRequest(DWORD protocol) : protocol(protocol) {}

  SCARD_IO_REQUEST AsSCardIoRequest() const;

  static SCardIoRequest FromSCardIoRequest(const SCARD_IO_REQUEST& value);

  DWORD protocol;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_SRC_PUBLIC_SCARD_STRUCTS_SERIALIZATION_H_
