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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CPP_CLIENT_PCSC_LITE_OVER_REQUESTER_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CPP_CLIENT_PCSC_LITE_OVER_REQUESTER_H_

#include <memory>

#include <pcsclite.h>
#include <winscard.h>
#include <wintypes.h>

#include "common/cpp/src/public/requesting/remote_call_adaptor.h"
#include "common/cpp/src/public/requesting/request_result.h"
#include "common/cpp/src/public/requesting/requester.h"
#include "third_party/pcsc-lite/naclport/common/src/public/pcsc_lite.h"

namespace google_smart_card {

// The name of the requester that should be used for the requests made by the
// PcscLiteOverRequester class (see also the
// google_smart_card_common/requesting/requester.h header).
extern const char kPcscLiteRequesterName[];

// This class provides an implementation for the PC/SC-Lite client API that
// forwards all calls through the passed requester to its counterpart library
// (which is normally a JavaScript PC/SC-Lite client library that, it turn,
// forwards all requests to the server App - for the details, see the
// /third_party/pcsc-lite/naclport/js_client/ directory).
//
// The function arguments and the returned values are (de)serialized with the
// help of functions from the
// google_smart_card_pcsc_lite_common/scard_structs_serialization.h header.
class PcscLiteOverRequester final : public PcscLite {
 public:
  // Creates the instance with the specified requester.
  //
  // The passed requester should normally be created with the
  // kPcscLiteRequesterName name.
  explicit PcscLiteOverRequester(std::unique_ptr<Requester> requester);

  PcscLiteOverRequester(const PcscLiteOverRequester&) = delete;
  PcscLiteOverRequester& operator=(const PcscLiteOverRequester&) = delete;

  ~PcscLiteOverRequester() override;

  // Detaches the linked requester, which prevents making any further requests
  // through it and prevents waiting for the responses of the already started
  // requests.
  //
  // After this function call, the PC/SC-Lite client API functions are still
  // allowed to be called, but they will return errors instead of performing
  // the real requests.
  //
  // This function is primarily intended to be used during the Pepper module
  // shutdown process, for preventing the situations when some other threads
  // currently calling PC/SC-Lite client API functions or waiting for the finish
  // of the already called functions try to access the destroyed pp::Instance
  // object or some other associated objects.
  //
  // This function is safe to be called from any thread.
  void ShutDown();

  // PcscLite:
  LONG SCardEstablishContext(DWORD scope,
                             LPCVOID reserved_1,
                             LPCVOID reserved_2,
                             LPSCARDCONTEXT s_card_context) override;
  LONG SCardReleaseContext(SCARDCONTEXT s_card_context) override;
  LONG SCardConnect(SCARDCONTEXT s_card_context,
                    LPCSTR reader_name,
                    DWORD share_mode,
                    DWORD preferred_protocols,
                    LPSCARDHANDLE s_card_handle,
                    LPDWORD active_protocol) override;
  LONG SCardReconnect(SCARDHANDLE s_card_handle,
                      DWORD share_mode,
                      DWORD preferred_protocols,
                      DWORD initialization_action,
                      LPDWORD active_protocol) override;
  LONG SCardDisconnect(SCARDHANDLE s_card_handle, DWORD disposition) override;
  LONG SCardBeginTransaction(SCARDHANDLE s_card_handle) override;
  LONG SCardEndTransaction(SCARDHANDLE s_card_handle,
                           DWORD disposition_action) override;
  LONG SCardStatus(SCARDHANDLE s_card_handle,
                   LPSTR reader_name,
                   LPDWORD reader_name_length,
                   LPDWORD state,
                   LPDWORD protocol,
                   LPBYTE atr,
                   LPDWORD atr_length) override;
  LONG SCardGetStatusChange(SCARDCONTEXT s_card_context,
                            DWORD timeout,
                            SCARD_READERSTATE* reader_states,
                            DWORD reader_states_size) override;
  LONG SCardControl(SCARDHANDLE s_card_handle,
                    DWORD control_code,
                    LPCVOID send_buffer,
                    DWORD send_buffer_length,
                    LPVOID receive_buffer,
                    DWORD receive_buffer_length,
                    LPDWORD bytes_returned) override;
  LONG SCardGetAttrib(SCARDHANDLE s_card_handle,
                      DWORD attribute_id,
                      LPBYTE attribute_buffer,
                      LPDWORD attribute_buffer_length) override;
  LONG SCardSetAttrib(SCARDHANDLE s_card_handle,
                      DWORD attribute_id,
                      LPCBYTE attribute_buffer,
                      DWORD attribute_buffer_length) override;
  LONG SCardTransmit(SCARDHANDLE s_card_handle,
                     const SCARD_IO_REQUEST* send_protocol_information,
                     LPCBYTE send_buffer,
                     DWORD send_buffer_length,
                     SCARD_IO_REQUEST* receive_protocol_information,
                     LPBYTE receive_buffer,
                     LPDWORD receive_buffer_length) override;
  LONG SCardListReaders(SCARDCONTEXT s_card_context,
                        LPCSTR groups,
                        LPSTR readers,
                        LPDWORD readers_size) override;
  LONG SCardFreeMemory(SCARDCONTEXT s_card_context, LPCVOID memory) override;
  LONG SCardListReaderGroups(SCARDCONTEXT s_card_context,
                             LPSTR groups,
                             LPDWORD groups_size) override;
  LONG SCardCancel(SCARDCONTEXT s_card_context) override;
  LONG SCardIsValidContext(SCARDCONTEXT s_card_context) override;

 private:
  std::unique_ptr<Requester> requester_;
  RemoteCallAdaptor remote_call_adaptor_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_CPP_CLIENT_PCSC_LITE_OVER_REQUESTER_H_
