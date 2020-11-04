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

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_PCSC_LITE_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_PCSC_LITE_H_

#include <pcsclite.h>
#include <wintypes.h>

namespace google_smart_card {

// Interface corresponding to the PC/SC-Lite API.
//
// All functions presented here have the same signatures as the original
// PC/SC-Lite API functions (see the winscard.h header in the PC/SC-Lite
// sources and the documentation at
// <https://pcsclite.alioth.debian.org/api/group__API.html>).
//
// Note that the pcsc_stringify_error function (that in the original
// PC/SC-Lite sources is defined in the pcsclite.h header) is not present here:
// there is no reason to provide a polymorphic behavior to this simple mapping
// function.
class PcscLite {
 public:
  virtual ~PcscLite() = default;

  virtual LONG SCardEstablishContext(DWORD dwScope,
                                     LPCVOID pvReserved1,
                                     LPCVOID pvReserved2,
                                     LPSCARDCONTEXT phContext) = 0;

  virtual LONG SCardReleaseContext(SCARDCONTEXT hContext) = 0;

  virtual LONG SCardConnect(SCARDCONTEXT hContext,
                            LPCSTR szReader,
                            DWORD dwShareMode,
                            DWORD dwPreferredProtocols,
                            LPSCARDHANDLE phCard,
                            LPDWORD pdwActiveProtocol) = 0;

  virtual LONG SCardReconnect(SCARDHANDLE hCard,
                              DWORD dwShareMode,
                              DWORD dwPreferredProtocols,
                              DWORD dwInitialization,
                              LPDWORD pdwActiveProtocol) = 0;

  virtual LONG SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition) = 0;

  virtual LONG SCardBeginTransaction(SCARDHANDLE hCard) = 0;

  virtual LONG SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition) = 0;

  virtual LONG SCardStatus(SCARDHANDLE hCard,
                           LPSTR szReaderName,
                           LPDWORD pcchReaderLen,
                           LPDWORD pdwState,
                           LPDWORD pdwProtocol,
                           LPBYTE pbAtr,
                           LPDWORD pcbAtrLen) = 0;

  virtual LONG SCardGetStatusChange(SCARDCONTEXT hContext,
                                    DWORD dwTimeout,
                                    SCARD_READERSTATE* rgReaderStates,
                                    DWORD cReaders) = 0;

  virtual LONG SCardControl(SCARDHANDLE hCard,
                            DWORD dwControlCode,
                            LPCVOID pbSendBuffer,
                            DWORD cbSendLength,
                            LPVOID pbRecvBuffer,
                            DWORD cbRecvLength,
                            LPDWORD lpBytesReturned) = 0;

  virtual LONG SCardGetAttrib(SCARDHANDLE hCard,
                              DWORD dwAttrId,
                              LPBYTE pbAttr,
                              LPDWORD pcbAttrLen) = 0;

  virtual LONG SCardSetAttrib(SCARDHANDLE hCard,
                              DWORD dwAttrId,
                              LPCBYTE pbAttr,
                              DWORD cbAttrLen) = 0;

  virtual LONG SCardTransmit(SCARDHANDLE hCard,
                             const SCARD_IO_REQUEST* pioSendPci,
                             LPCBYTE pbSendBuffer,
                             DWORD cbSendLength,
                             SCARD_IO_REQUEST* pioRecvPci,
                             LPBYTE pbRecvBuffer,
                             LPDWORD pcbRecvLength) = 0;

  virtual LONG SCardListReaders(SCARDCONTEXT hContext,
                                LPCSTR mszGroups,
                                LPSTR mszReaders,
                                LPDWORD pcchReaders) = 0;

  virtual LONG SCardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem) = 0;

  virtual LONG SCardListReaderGroups(SCARDCONTEXT hContext,
                                     LPSTR mszGroups,
                                     LPDWORD pcchGroups) = 0;

  virtual LONG SCardCancel(SCARDCONTEXT hContext) = 0;

  virtual LONG SCardIsValidContext(SCARDCONTEXT hContext) = 0;
};

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_PCSC_LITE_H_
