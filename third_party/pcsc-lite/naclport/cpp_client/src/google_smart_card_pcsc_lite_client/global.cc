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

#include "third_party/pcsc-lite/naclport/cpp_client/src/google_smart_card_pcsc_lite_client/global.h"

#include <string>
#include <utility>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/requesting/js_requester.h"
#include "common/cpp/src/public/requesting/requester.h"
#include "common/cpp/src/public/unique_ptr_utils.h"
#include "third_party/pcsc-lite/naclport/common/src/public/pcsc_lite.h"
#include "third_party/pcsc-lite/naclport/common/src/public/pcsc_lite_tracing_wrapper.h"
#include "third_party/pcsc-lite/naclport/cpp_client/src/pcsc_lite_over_requester.h"

namespace {

constexpr char kLoggingPrefix[] = "[PC/SC-Lite client] ";

// A unique global pointer to an implementation of PcscLite interface that is
// used by the implementation of global PC/SC-Lite API functions in this
// library.
google_smart_card::PcscLite* g_pcsc_lite = nullptr;

google_smart_card::PcscLite* GetGlobalPcscLite() {
  GOOGLE_SMART_CARD_CHECK(g_pcsc_lite);
  return g_pcsc_lite;
}

}  // namespace

namespace google_smart_card {

class PcscLiteOverRequesterGlobal::Impl final {
 public:
  Impl(GlobalContext* global_context, TypedMessageRouter* typed_message_router)
      : pcsc_lite_over_requester_(
            MakeUnique<JsRequester>(kPcscLiteRequesterName,
                                    global_context,
                                    typed_message_router)) {
#ifndef NDEBUG
    pcsc_lite_tracing_wrapper_.reset(
        new PcscLiteTracingWrapper(&pcsc_lite_over_requester_, kLoggingPrefix));
#endif  // NDEBUG
  }

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;
  ~Impl() = default;

  void ShutDown() { pcsc_lite_over_requester_.ShutDown(); }

  PcscLite* pcsc_lite() {
    if (pcsc_lite_tracing_wrapper_)
      return pcsc_lite_tracing_wrapper_.get();
    return &pcsc_lite_over_requester_;
  }

 private:
  PcscLiteOverRequester pcsc_lite_over_requester_;
  std::unique_ptr<PcscLiteTracingWrapper> pcsc_lite_tracing_wrapper_;
};

PcscLiteOverRequesterGlobal::PcscLiteOverRequesterGlobal(
    GlobalContext* global_context,
    TypedMessageRouter* typed_message_router)
    : impl_(MakeUnique<Impl>(global_context, typed_message_router)) {
  GOOGLE_SMART_CARD_CHECK(!g_pcsc_lite);
  g_pcsc_lite = impl_->pcsc_lite();
}

PcscLiteOverRequesterGlobal::~PcscLiteOverRequesterGlobal() {
  GOOGLE_SMART_CARD_CHECK(g_pcsc_lite == impl_->pcsc_lite());
  g_pcsc_lite = nullptr;
}

void PcscLiteOverRequesterGlobal::ShutDown() {
  impl_->ShutDown();
}

}  // namespace google_smart_card

LONG SCardEstablishContext(DWORD dwScope,
                           LPCVOID pvReserved1,
                           LPCVOID pvReserved2,
                           LPSCARDCONTEXT phContext) {
  return GetGlobalPcscLite()->SCardEstablishContext(dwScope, pvReserved1,
                                                    pvReserved2, phContext);
}

LONG SCardReleaseContext(SCARDCONTEXT hContext) {
  return GetGlobalPcscLite()->SCardReleaseContext(hContext);
}

LONG SCardConnect(SCARDCONTEXT hContext,
                  LPCSTR szReader,
                  DWORD dwShareMode,
                  DWORD dwPreferredProtocols,
                  LPSCARDHANDLE phCard,
                  LPDWORD pdwActiveProtocol) {
  return GetGlobalPcscLite()->SCardConnect(hContext, szReader, dwShareMode,
                                           dwPreferredProtocols, phCard,
                                           pdwActiveProtocol);
}

LONG SCardReconnect(SCARDHANDLE hCard,
                    DWORD dwShareMode,
                    DWORD dwPreferredProtocols,
                    DWORD dwInitialization,
                    LPDWORD pdwActiveProtocol) {
  return GetGlobalPcscLite()->SCardReconnect(
      hCard, dwShareMode, dwPreferredProtocols, dwInitialization,
      pdwActiveProtocol);
}

LONG SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition) {
  return GetGlobalPcscLite()->SCardDisconnect(hCard, dwDisposition);
}

LONG SCardBeginTransaction(SCARDHANDLE hCard) {
  return GetGlobalPcscLite()->SCardBeginTransaction(hCard);
}

LONG SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition) {
  return GetGlobalPcscLite()->SCardEndTransaction(hCard, dwDisposition);
}

LONG SCardStatus(SCARDHANDLE hCard,
                 LPSTR szReaderName,
                 LPDWORD pcchReaderLen,
                 LPDWORD pdwState,
                 LPDWORD pdwProtocol,
                 LPBYTE pbAtr,
                 LPDWORD pcbAtrLen) {
  return GetGlobalPcscLite()->SCardStatus(hCard, szReaderName, pcchReaderLen,
                                          pdwState, pdwProtocol, pbAtr,
                                          pcbAtrLen);
}

LONG SCardGetStatusChange(SCARDCONTEXT hContext,
                          DWORD dwTimeout,
                          SCARD_READERSTATE* rgReaderStates,
                          DWORD cReaders) {
  return GetGlobalPcscLite()->SCardGetStatusChange(hContext, dwTimeout,
                                                   rgReaderStates, cReaders);
}

LONG SCardControl(SCARDHANDLE hCard,
                  DWORD dwControlCode,
                  LPCVOID pbSendBuffer,
                  DWORD cbSendLength,
                  LPVOID pbRecvBuffer,
                  DWORD cbRecvLength,
                  LPDWORD lpBytesReturned) {
  return GetGlobalPcscLite()->SCardControl(hCard, dwControlCode, pbSendBuffer,
                                           cbSendLength, pbRecvBuffer,
                                           cbRecvLength, lpBytesReturned);
}

LONG SCardGetAttrib(SCARDHANDLE hCard,
                    DWORD dwAttrId,
                    LPBYTE pbAttr,
                    LPDWORD pcbAttrLen) {
  return GetGlobalPcscLite()->SCardGetAttrib(hCard, dwAttrId, pbAttr,
                                             pcbAttrLen);
}

LONG SCardSetAttrib(SCARDHANDLE hCard,
                    DWORD dwAttrId,
                    LPCBYTE pbAttr,
                    DWORD cbAttrLen) {
  return GetGlobalPcscLite()->SCardSetAttrib(hCard, dwAttrId, pbAttr,
                                             cbAttrLen);
}

LONG SCardTransmit(SCARDHANDLE hCard,
                   const SCARD_IO_REQUEST* pioSendPci,
                   LPCBYTE pbSendBuffer,
                   DWORD cbSendLength,
                   SCARD_IO_REQUEST* pioRecvPci,
                   LPBYTE pbRecvBuffer,
                   LPDWORD pcbRecvLength) {
  return GetGlobalPcscLite()->SCardTransmit(hCard, pioSendPci, pbSendBuffer,
                                            cbSendLength, pioRecvPci,
                                            pbRecvBuffer, pcbRecvLength);
}

LONG SCardListReaders(SCARDCONTEXT hContext,
                      LPCSTR mszGroups,
                      LPSTR mszReaders,
                      LPDWORD pcchReaders) {
  return GetGlobalPcscLite()->SCardListReaders(hContext, mszGroups, mszReaders,
                                               pcchReaders);
}

LONG SCardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem) {
  return GetGlobalPcscLite()->SCardFreeMemory(hContext, pvMem);
}

LONG SCardListReaderGroups(SCARDCONTEXT hContext,
                           LPSTR mszGroups,
                           LPDWORD pcchGroups) {
  return GetGlobalPcscLite()->SCardListReaderGroups(hContext, mszGroups,
                                                    pcchGroups);
}

LONG SCardCancel(SCARDCONTEXT hContext) {
  return GetGlobalPcscLite()->SCardCancel(hContext);
}

LONG SCardIsValidContext(SCARDCONTEXT hContext) {
  return GetGlobalPcscLite()->SCardIsValidContext(hContext);
}
