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

#include <google_smart_card_common/pp_var_utils/struct_converter.h>

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

// static
template <>
constexpr const char*
StructConverter<InboundSCardReaderState>::GetStructTypeName() {
  return "SCARD_READERSTATE_inbound";
}

// static
template <>
template <typename Callback>
void StructConverter<InboundSCardReaderState>::VisitFields(
    const InboundSCardReaderState& value, Callback callback) {
  callback(&value.reader_name, "reader_name");
  callback(&value.user_data, "user_data");
  callback(&value.current_state, "current_state");
}

// static
template <>
constexpr const char*
StructConverter<OutboundSCardReaderState>::GetStructTypeName() {
  return "SCARD_READERSTATE_outbound";
}

// static
template <>
template <typename Callback>
void StructConverter<OutboundSCardReaderState>::VisitFields(
    const OutboundSCardReaderState& value, Callback callback) {
  callback(&value.reader_name, "reader_name");
  callback(&value.user_data, "user_data");
  callback(&value.current_state, "current_state");
  callback(&value.event_state, "event_state");
  callback(&value.atr, "atr");
}

// static
template <>
constexpr const char*
StructConverter<SCardIoRequest>::GetStructTypeName() {
  return "SCARD_IO_REQUEST";
}

// static
template <>
template <typename Callback>
void StructConverter<SCardIoRequest>::VisitFields(
    const SCardIoRequest& value, Callback callback) {
  callback(&value.protocol, "protocol");
}

// static
InboundSCardReaderState InboundSCardReaderState::FromSCardReaderState(
    const SCARD_READERSTATE& value) {
  GOOGLE_SMART_CARD_CHECK(value.szReader);
  return InboundSCardReaderState(
      value.szReader,
      value.pvUserData ?
          reinterpret_cast<uintptr_t>(value.pvUserData) : optional<uintptr_t>(),
      value.dwCurrentState);
}

// static
OutboundSCardReaderState OutboundSCardReaderState::FromSCardReaderState(
    const SCARD_READERSTATE& value) {
  GOOGLE_SMART_CARD_CHECK(value.szReader);
  return OutboundSCardReaderState(
      value.szReader,
      value.pvUserData ?
          reinterpret_cast<uintptr_t>(value.pvUserData) : optional<uintptr_t>(),
      value.dwCurrentState,
      value.dwEventState,
      GetSCardReaderStateAtr(value));
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

template <>
pp::Var MakeVar(const SCARD_READERSTATE& value) {
  VarDictBuilder result_builder;
  GOOGLE_SMART_CARD_CHECK(value.szReader);
  result_builder.Add("reader_name", value.szReader);
  if (value.pvUserData) {
    result_builder.Add(
        "user_data", reinterpret_cast<uintptr_t>(value.pvUserData));
  }
  result_builder.Add("current_state", value.dwCurrentState);
  result_builder.Add("event_state", value.dwEventState);
  // Chrome Extensions API does not allow sending ArrayBuffers in message
  // fields, so instead of pp::VarArrayBuffer a pp::VarArray with the bytes as
  // its element is constructed.
  result_builder.Add("atr", MakeVar(GetSCardReaderStateAtr(value)));
  return result_builder.Result();
}

bool VarAs(
    const pp::Var& var,
    InboundSCardReaderState* result,
    std::string* error_message) {
  return StructConverter<InboundSCardReaderState>::ConvertFromVar(
      var, result, error_message);
}

template <>
pp::Var MakeVar(const InboundSCardReaderState& value) {
  return StructConverter<InboundSCardReaderState>::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var,
    OutboundSCardReaderState* result,
    std::string* error_message) {
  return StructConverter<OutboundSCardReaderState>::ConvertFromVar(
      var, result, error_message);
}

template <>
pp::Var MakeVar(const OutboundSCardReaderState& value) {
  return StructConverter<OutboundSCardReaderState>::ConvertToVar(value);
}

bool VarAs(
    const pp::Var& var, SCardIoRequest* result, std::string* error_message) {
  return StructConverter<SCardIoRequest>::ConvertFromVar(
      var, result, error_message);
}

template <>
pp::Var MakeVar(const SCardIoRequest& value) {
  return StructConverter<SCardIoRequest>::ConvertToVar(value);
}

}  // namespace google_smart_card
