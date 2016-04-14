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

// FIXME(emaxx): Write docs.

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CPP_CLIENT_PCSC_LITE_OVER_REQUESTER_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CPP_CLIENT_PCSC_LITE_OVER_REQUESTER_H_

#include <memory>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include <google_smart_card_common/requesting/requester.h>
#include <google_smart_card_common/requesting/request_result.h>
#include <google_smart_card_common/requesting/remote_call_adaptor.h>
#include <google_smart_card_pcsc_lite_common/pcsc_lite.h>

namespace google_smart_card {

constexpr char kPcscLiteRequesterName[] = "pcsc_lite";

class PcscLiteOverRequester final : public PcscLite {
 public:
  explicit PcscLiteOverRequester(std::unique_ptr<Requester> requester);

  void Detach();

  LONG SCardEstablishContext(
      DWORD scope,
      LPCVOID reserved_1,
      LPCVOID reserved_2,
      LPSCARDCONTEXT s_card_context) override;

  LONG SCardReleaseContext(SCARDCONTEXT s_card_context) override;

  LONG SCardConnect(
      SCARDCONTEXT s_card_context,
      LPCSTR reader_name,
      DWORD share_mode,
      DWORD preferred_protocols,
      LPSCARDHANDLE s_card_handle,
      LPDWORD active_protocol) override;

  LONG SCardReconnect(
      SCARDHANDLE s_card_handle,
      DWORD share_mode,
      DWORD preferred_protocols,
      DWORD initialization_action,
      LPDWORD active_protocol) override;

  LONG SCardDisconnect(SCARDHANDLE s_card_handle, DWORD disposition) override;

  LONG SCardBeginTransaction(SCARDHANDLE s_card_handle) override;

  LONG SCardEndTransaction(
      SCARDHANDLE s_card_handle, DWORD disposition_action) override;

  LONG SCardStatus(
      SCARDHANDLE s_card_handle,
      LPSTR reader_name,
      LPDWORD reader_name_length,
      LPDWORD state,
      LPDWORD protocol,
      LPBYTE atr,
      LPDWORD atr_length) override;

  LONG SCardGetStatusChange(
      SCARDCONTEXT s_card_context,
      DWORD timeout,
      SCARD_READERSTATE* reader_states,
      DWORD reader_states_size) override;

  LONG SCardControl(
      SCARDHANDLE s_card_handle,
      DWORD control_code,
      LPCVOID send_buffer,
      DWORD send_buffer_length,
      LPVOID receive_buffer,
      DWORD receive_buffer_length,
      LPDWORD bytes_returned) override;

  LONG SCardGetAttrib(
      SCARDHANDLE s_card_handle,
      DWORD attribute_id,
      LPBYTE attribute_buffer,
      LPDWORD attribute_buffer_length) override;

  LONG SCardSetAttrib(
      SCARDHANDLE s_card_handle,
      DWORD attribute_id,
      LPCBYTE attribute_buffer,
      DWORD attribute_buffer_length) override;

  LONG SCardTransmit(
      SCARDHANDLE s_card_handle,
      const SCARD_IO_REQUEST* send_protocol_information,
      LPCBYTE send_buffer,
      DWORD send_buffer_length,
      SCARD_IO_REQUEST* receive_protocol_information,
      LPBYTE receive_buffer,
      LPDWORD receive_buffer_length) override;

  LONG SCardListReaders(
      SCARDCONTEXT s_card_context,
      LPCSTR groups,
      LPSTR readers,
      LPDWORD readers_size) override;

  LONG SCardFreeMemory(SCARDCONTEXT s_card_context, LPCVOID memory) override;

  LONG SCardListReaderGroups(
      SCARDCONTEXT s_card_context, LPSTR groups, LPDWORD groups_size) override;

  LONG SCardCancel(SCARDCONTEXT s_card_context) override;

  LONG SCardIsValidContext(SCARDCONTEXT s_card_context) override;

 private:
  template <typename ... Results>
  static LONG ExtractRequestResultsAndCode(
      const GenericRequestResult& generic_request_result, Results* ... results);

  std::unique_ptr<Requester> requester_;
  RemoteCallAdaptor remote_call_adaptor_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CPP_CLIENT_PCSC_LITE_OVER_REQUESTER_H_
