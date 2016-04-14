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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_PCSC_LITE_TRACING_WRAPPER_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_PCSC_LITE_TRACING_WRAPPER_H_

#include <string>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include <google_smart_card_pcsc_lite_common/pcsc_lite.h>

namespace google_smart_card {

// Wrapper that adds debug tracing of the called PC/SC functions.
class PcscLiteTracingWrapper final : public PcscLite {
 public:
  explicit PcscLiteTracingWrapper(PcscLite* pcsc_lite);
  PcscLiteTracingWrapper(
      PcscLite* pcsc_lite, const std::string& logging_prefix);

  LONG SCardEstablishContext(
      DWORD dwScope,
      LPCVOID pvReserved1,
      LPCVOID pvReserved2,
      LPSCARDCONTEXT phContext) override;

  LONG SCardReleaseContext(SCARDCONTEXT hContext) override;

  LONG SCardConnect(
      SCARDCONTEXT hContext,
      LPCSTR szReader,
      DWORD dwShareMode,
      DWORD dwPreferredProtocols,
      LPSCARDHANDLE phCard,
      LPDWORD pdwActiveProtocol) override;

  LONG SCardReconnect(
      SCARDHANDLE hCard,
      DWORD dwShareMode,
      DWORD dwPreferredProtocols,
      DWORD dwInitialization,
      LPDWORD pdwActiveProtocol) override;

  LONG SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition) override;

  LONG SCardBeginTransaction(SCARDHANDLE hCard) override;

  LONG SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition) override;

  LONG SCardStatus(
      SCARDHANDLE hCard,
      LPSTR szReaderNames,
      LPDWORD pcchReaderLen,
      LPDWORD pdwState,
      LPDWORD pdwProtocol,
      LPBYTE pbAtr,
      LPDWORD pcbAtrLen) override;

  LONG SCardGetStatusChange(
      SCARDCONTEXT hContext,
      DWORD dwTimeout,
      SCARD_READERSTATE* rgReaderStates,
      DWORD cReaders) override;

  LONG SCardControl(
      SCARDHANDLE hCard,
      DWORD dwControlCode,
      LPCVOID pbSendBuffer,
      DWORD cbSendLength,
      LPVOID pbRecvBuffer,
      DWORD cbRecvLength,
      LPDWORD lpBytesReturned) override;

  LONG SCardGetAttrib(
      SCARDHANDLE hCard,
      DWORD dwAttrId,
      LPBYTE pbAttr,
      LPDWORD pcbAttrLen) override;

  LONG SCardSetAttrib(
      SCARDHANDLE hCard,
      DWORD dwAttrId,
      LPCBYTE pbAttr,
      DWORD cbAttrLen) override;

  LONG SCardTransmit(
      SCARDHANDLE hCard,
      const SCARD_IO_REQUEST* pioSendPci,
      LPCBYTE pbSendBuffer,
      DWORD cbSendLength,
      SCARD_IO_REQUEST* pioRecvPci,
      LPBYTE pbRecvBuffer,
      LPDWORD pcbRecvLength) override;

  LONG SCardListReaders(
      SCARDCONTEXT hContext,
      LPCSTR mszGroups,
      LPSTR mszReaders,
      LPDWORD pcchReaders) override;

  LONG SCardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem) override;

  LONG SCardListReaderGroups(
      SCARDCONTEXT hContext, LPSTR mszGroups, LPDWORD pcchGroups) override;

  LONG SCardCancel(SCARDCONTEXT hContext) override;

  LONG SCardIsValidContext(SCARDCONTEXT hContext) override;

 private:
  PcscLite* const pcsc_lite_;
  const std::string logging_prefix_;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_PCSC_LITE_TRACING_WRAPPER_H_
