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

#include <google_smart_card_pcsc_lite_common/scard_structs_serialization.h>

#include <cstring>

#include <google_smart_card_common/value_conversion.h>

namespace google_smart_card {

namespace {

std::vector<uint8_t> GetSCardReaderStateAtr(
    const SCARD_READERSTATE& s_card_reader_state) {
  if (!s_card_reader_state.cbAtr)
    return {};
  GOOGLE_SMART_CARD_CHECK(s_card_reader_state.cbAtr <= MAX_ATR_SIZE);
  return std::vector<uint8_t>(
      s_card_reader_state.rgbAtr,
      s_card_reader_state.rgbAtr + s_card_reader_state.cbAtr);
}

}  // namespace

template <>
StructValueDescriptor<InboundSCardReaderState>::Description
StructValueDescriptor<InboundSCardReaderState>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names of
  // SCARD_READERSTATE_IN in
  // //third_party/pcsc-lite/naclport/js_client/src/api.js.
  return Describe("SCARD_READERSTATE_inbound")
      .WithField(&InboundSCardReaderState::reader_name, "reader_name")
      .WithField(&InboundSCardReaderState::user_data, "user_data")
      .WithField(&InboundSCardReaderState::current_state, "current_state");
}

template <>
StructValueDescriptor<OutboundSCardReaderState>::Description
StructValueDescriptor<OutboundSCardReaderState>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names of
  // SCARD_READERSTATE_OUT in
  // //third_party/pcsc-lite/naclport/js_client/src/api.js.
  return Describe("SCARD_READERSTATE_outbound")
      .WithField(&OutboundSCardReaderState::reader_name, "reader_name")
      .WithField(&OutboundSCardReaderState::user_data, "user_data")
      .WithField(&OutboundSCardReaderState::current_state, "current_state")
      .WithField(&OutboundSCardReaderState::event_state, "event_state")
      .WithField(&OutboundSCardReaderState::atr, "atr");
}

template <>
StructValueDescriptor<SCardIoRequest>::Description
StructValueDescriptor<SCardIoRequest>::GetDescription() {
  // Note: Strings passed to WithField() below must match the property names of
  // SCARD_IO_REQUEST in //third_party/pcsc-lite/naclport/js_client/src/api.js.
  return Describe("SCARD_IO_REQUEST")
      .WithField(&SCardIoRequest::protocol, "protocol");
}

// static
InboundSCardReaderState InboundSCardReaderState::FromSCardReaderState(
    const SCARD_READERSTATE& value) {
  GOOGLE_SMART_CARD_CHECK(value.szReader);
  return InboundSCardReaderState(
      value.szReader,
      value.pvUserData ? reinterpret_cast<uintptr_t>(value.pvUserData)
                       : optional<uintptr_t>(),
      value.dwCurrentState);
}

// static
OutboundSCardReaderState OutboundSCardReaderState::FromSCardReaderState(
    const SCARD_READERSTATE& value) {
  GOOGLE_SMART_CARD_CHECK(value.szReader);
  return OutboundSCardReaderState(
      value.szReader,
      value.pvUserData ? reinterpret_cast<uintptr_t>(value.pvUserData)
                       : optional<uintptr_t>(),
      value.dwCurrentState, value.dwEventState, GetSCardReaderStateAtr(value));
}

SCARD_IO_REQUEST SCardIoRequest::AsSCardIoRequest() const {
  SCARD_IO_REQUEST result;
  result.dwProtocol = protocol;
  result.cbPciLength = sizeof(SCARD_IO_REQUEST);
  return result;
}

// static
SCardIoRequest SCardIoRequest::FromSCardIoRequest(
    const SCARD_IO_REQUEST& value) {
  return SCardIoRequest(value.dwProtocol);
}

}  // namespace google_smart_card
