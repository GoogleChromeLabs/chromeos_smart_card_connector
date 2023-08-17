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

#include "third_party/pcsc-lite/naclport/common/src/public/pcsc_lite_tracing_wrapper.h"

#include <reader.h>

#include "common/cpp/src/public/logging/function_call_tracer.h"
#include "common/cpp/src/public/logging/hex_dumping.h"
#include "third_party/pcsc-lite/naclport/common/src/public/scard_debug_dump.h"

namespace google_smart_card {

PcscLiteTracingWrapper::PcscLiteTracingWrapper(
    PcscLite* pcsc_lite,
    const std::string& logging_prefix,
    LogSeverity log_severity)
    : pcsc_lite_(pcsc_lite),
      logging_prefix_(logging_prefix),
      log_severity_(log_severity) {
  GOOGLE_SMART_CARD_CHECK(pcsc_lite_);
}

PcscLiteTracingWrapper::~PcscLiteTracingWrapper() = default;

LONG PcscLiteTracingWrapper::SCardEstablishContext(DWORD dwScope,
                                                   LPCVOID pvReserved1,
                                                   LPCVOID pvReserved2,
                                                   LPSCARDCONTEXT phContext) {
  FunctionCallTracer tracer("SCardEstablishContext", logging_prefix_,
                            log_severity_);
  tracer.AddPassedArg("dwScope", DebugDumpSCardScope(dwScope));
  tracer.AddPassedArg("pvReserved1", HexDumpPointer(pvReserved1));
  tracer.AddPassedArg("pvReserved2", HexDumpPointer(pvReserved2));
  tracer.AddPassedArg("phContext", HexDumpPointer(phContext));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardEstablishContext(
      dwScope, pvReserved1, pvReserved2, phContext);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    if (phContext) {
      tracer.AddReturnedArg("*phContext", DebugDumpSCardContext(*phContext));
    }
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardReleaseContext(SCARDCONTEXT hContext) {
  FunctionCallTracer tracer("SCardReleaseContext", logging_prefix_,
                            log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardScope(hContext));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardReleaseContext(hContext);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardConnect(SCARDCONTEXT hContext,
                                          LPCSTR szReader,
                                          DWORD dwShareMode,
                                          DWORD dwPreferredProtocols,
                                          LPSCARDHANDLE phCard,
                                          LPDWORD pdwActiveProtocol) {
  FunctionCallTracer tracer("SCardConnect", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(hContext));
  tracer.AddPassedArg("szReader", DebugDumpSCardCString(szReader));
  tracer.AddPassedArg("dwShareMode", DebugDumpSCardShareMode(dwShareMode));
  tracer.AddPassedArg("dwPreferredProtocols",
                      DebugDumpSCardProtocols(dwPreferredProtocols));
  tracer.AddPassedArg("phCard", HexDumpPointer(phCard));
  tracer.AddPassedArg("pdwActiveProtocol", HexDumpPointer(pdwActiveProtocol));
  tracer.LogEntrance();

  const LONG return_code =
      pcsc_lite_->SCardConnect(hContext, szReader, dwShareMode,
                               dwPreferredProtocols, phCard, pdwActiveProtocol);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    if (phCard)
      tracer.AddReturnedArg("*phCard", DebugDumpSCardHandle(*phCard));
    if (pdwActiveProtocol) {
      tracer.AddReturnedArg("*pdwActiveProtocol",
                            DebugDumpSCardProtocol(*pdwActiveProtocol));
    }
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardReconnect(SCARDHANDLE hCard,
                                            DWORD dwShareMode,
                                            DWORD dwPreferredProtocols,
                                            DWORD dwInitialization,
                                            LPDWORD pdwActiveProtocol) {
  FunctionCallTracer tracer("SCardReconnect", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("dwShareMode", DebugDumpSCardShareMode(dwShareMode));
  tracer.AddPassedArg("dwPreferredProtocols",
                      DebugDumpSCardProtocols(dwPreferredProtocols));
  tracer.AddPassedArg("dwInitialization",
                      DebugDumpSCardDisposition(dwInitialization));
  tracer.AddPassedArg("pdwActiveProtocol", HexDumpPointer(pdwActiveProtocol));
  tracer.LogEntrance();

  const LONG return_code =
      pcsc_lite_->SCardReconnect(hCard, dwShareMode, dwPreferredProtocols,
                                 dwInitialization, pdwActiveProtocol);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    if (pdwActiveProtocol) {
      tracer.AddReturnedArg("*pdwActiveProtocol",
                            DebugDumpSCardProtocol(*pdwActiveProtocol));
    }
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardDisconnect(SCARDHANDLE hCard,
                                             DWORD dwDisposition) {
  FunctionCallTracer tracer("SCardDisconnect", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("dwDisposition",
                      DebugDumpSCardDisposition(dwDisposition));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardDisconnect(hCard, dwDisposition);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardBeginTransaction(SCARDHANDLE hCard) {
  FunctionCallTracer tracer("SCardBeginTransaction", logging_prefix_,
                            log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardBeginTransaction(hCard);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardEndTransaction(SCARDHANDLE hCard,
                                                 DWORD dwDisposition) {
  FunctionCallTracer tracer("SCardEndTransaction", logging_prefix_,
                            log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("dwDisposition",
                      DebugDumpSCardDisposition(dwDisposition));
  tracer.LogEntrance();

  const LONG return_code =
      pcsc_lite_->SCardEndTransaction(hCard, dwDisposition);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardStatus(SCARDHANDLE hCard,
                                         LPSTR szReaderName,
                                         LPDWORD pcchReaderLen,
                                         LPDWORD pdwState,
                                         LPDWORD pdwProtocol,
                                         LPBYTE pbAtr,
                                         LPDWORD pcbAtrLen) {
  FunctionCallTracer tracer("SCardStatus", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("szReaderName", HexDumpPointer(szReaderName));
  tracer.AddPassedArg("pcchReaderLen",
                      DebugDumpSCardBufferSizeInputPointer(pcchReaderLen));
  tracer.AddPassedArg("pdwState", HexDumpPointer(pdwState));
  tracer.AddPassedArg("pdwProtocol", HexDumpPointer(pdwProtocol));
  tracer.AddPassedArg("pbAtr", HexDumpPointer(pbAtr));
  tracer.AddPassedArg("pcbAtrLen",
                      DebugDumpSCardBufferSizeInputPointer(pcbAtrLen));
  tracer.LogEntrance();
  const bool is_reader_name_auto_allocation =
      pcchReaderLen && *pcchReaderLen == SCARD_AUTOALLOCATE;
  const bool is_atr_auto_allocation =
      pcbAtrLen && *pcbAtrLen == SCARD_AUTOALLOCATE;

  const LONG return_code =
      pcsc_lite_->SCardStatus(hCard, szReaderName, pcchReaderLen, pdwState,
                              pdwProtocol, pbAtr, pcbAtrLen);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS ||
      return_code == SCARD_E_INSUFFICIENT_BUFFER) {
    if (szReaderName && return_code == SCARD_S_SUCCESS) {
      tracer.AddReturnedArg("*szReaderName",
                            DebugDumpSCardOutputCStringBuffer(
                                szReaderName, is_reader_name_auto_allocation));
    }
    if (pcchReaderLen)
      tracer.AddReturnedArg("*pcchReaderLen", std::to_string(*pcchReaderLen));
    if (pdwState)
      tracer.AddReturnedArg("*pdwState", DebugDumpSCardState(*pdwState));
    if (pdwProtocol) {
      tracer.AddReturnedArg("*pdwProtocol",
                            DebugDumpSCardProtocol(*pdwProtocol));
    }
    if (pbAtr && return_code == SCARD_S_SUCCESS) {
      tracer.AddReturnedArg(
          "*pbAtr",
          DebugDumpSCardOutputBuffer(pbAtr, pcbAtrLen, is_atr_auto_allocation));
    }
    if (pcbAtrLen)
      tracer.AddReturnedArg("*pcbAtrLen", std::to_string(*pcbAtrLen));
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardGetStatusChange(
    SCARDCONTEXT hContext,
    DWORD dwTimeout,
    SCARD_READERSTATE* rgReaderStates,
    DWORD cReaders) {
  FunctionCallTracer tracer("SCardGetStatusChange", logging_prefix_,
                            log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(hContext));
  tracer.AddPassedArg("dwTimeout", std::to_string(dwTimeout));
  tracer.AddPassedArg("rgReaderStates", DebugDumpSCardInputReaderStates(
                                            rgReaderStates, cReaders));
  tracer.AddPassedArg("cReaders", std::to_string(cReaders));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardGetStatusChange(
      hContext, dwTimeout, rgReaderStates, cReaders);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    tracer.AddReturnedArg("*rgReaderStates", DebugDumpSCardOutputReaderStates(
                                                 rgReaderStates, cReaders));
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardControl(SCARDHANDLE hCard,
                                          DWORD dwControlCode,
                                          LPCVOID pbSendBuffer,
                                          DWORD cbSendLength,
                                          LPVOID pbRecvBuffer,
                                          DWORD cbRecvLength,
                                          LPDWORD lpBytesReturned) {
  FunctionCallTracer tracer("SCardControl", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("dwControlCode",
                      DebugDumpSCardControlCode(dwControlCode));
  tracer.AddPassedArg("pbSendBuffer",
                      DebugDumpSCardInputBuffer(pbSendBuffer, cbSendLength));
  tracer.AddPassedArg("cbSendLength", std::to_string(cbSendLength));
  tracer.AddPassedArg("pbRecvBuffer", HexDumpPointer(pbRecvBuffer));
  tracer.AddPassedArg("cbRecvLength", std::to_string(cbRecvLength));
  tracer.AddPassedArg("lpBytesReturned", HexDumpPointer(lpBytesReturned));
  tracer.LogEntrance();

  const LONG return_code =
      pcsc_lite_->SCardControl(hCard, dwControlCode, pbSendBuffer, cbSendLength,
                               pbRecvBuffer, cbRecvLength, lpBytesReturned);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS) {
    if (lpBytesReturned) {
      tracer.AddReturnedArg(
          "*pbRecvBuffer",
          DebugDumpSCardOutputBuffer(pbRecvBuffer, *lpBytesReturned));
    }
  }
  if (lpBytesReturned) {
    // It's correct to read the value pointed by this argument even in case of
    // errors, as, according to PC/SC-Lite and CCID sources, this output
    // parameter is always set (and the NaCl port of PC/SC-Lite conforms to this
    // too).
    tracer.AddReturnedArg("*lpBytesReturned", std::to_string(*lpBytesReturned));
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardGetAttrib(SCARDHANDLE hCard,
                                            DWORD dwAttrId,
                                            LPBYTE pbAttr,
                                            LPDWORD pcbAttrLen) {
  FunctionCallTracer tracer("SCardGetAttrib", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("dwAttrId", DebugDumpSCardAttributeId(dwAttrId));
  tracer.AddPassedArg("pbAttr", HexDumpPointer(pbAttr));
  tracer.AddPassedArg("pcbAttrLen",
                      DebugDumpSCardBufferSizeInputPointer(pcbAttrLen));
  tracer.LogEntrance();
  const bool is_auto_allocation =
      pcbAttrLen && *pcbAttrLen == SCARD_AUTOALLOCATE;

  const LONG return_code =
      pcsc_lite_->SCardGetAttrib(hCard, dwAttrId, pbAttr, pcbAttrLen);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS ||
      return_code == SCARD_E_INSUFFICIENT_BUFFER) {
    if (pbAttr && return_code == SCARD_S_SUCCESS) {
      tracer.AddReturnedArg(
          "*pbAttr",
          DebugDumpSCardOutputBuffer(pbAttr, pcbAttrLen, is_auto_allocation));
    }
    if (pcbAttrLen)
      tracer.AddReturnedArg("*pcbAttrLen", std::to_string(*pcbAttrLen));
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardSetAttrib(SCARDHANDLE hCard,
                                            DWORD dwAttrId,
                                            LPCBYTE pbAttr,
                                            DWORD cbAttrLen) {
  FunctionCallTracer tracer("SCardSetAttrib", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("dwAttrId", DebugDumpSCardAttributeId(dwAttrId));
  tracer.AddPassedArg("pbAttr", DebugDumpSCardInputBuffer(pbAttr, cbAttrLen));
  tracer.AddPassedArg("cbAttrLen", std::to_string(cbAttrLen));
  tracer.LogEntrance();

  const LONG return_code =
      pcsc_lite_->SCardSetAttrib(hCard, dwAttrId, pbAttr, cbAttrLen);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardTransmit(SCARDHANDLE hCard,
                                           const SCARD_IO_REQUEST* pioSendPci,
                                           LPCBYTE pbSendBuffer,
                                           DWORD cbSendLength,
                                           SCARD_IO_REQUEST* pioRecvPci,
                                           LPBYTE pbRecvBuffer,
                                           LPDWORD pcbRecvLength) {
  FunctionCallTracer tracer("SCardTransmit", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hCard", DebugDumpSCardHandle(hCard));
  tracer.AddPassedArg("pioSendPci", DebugDumpSCardIoRequest(pioSendPci));
  tracer.AddPassedArg("pbSendBuffer",
                      DebugDumpSCardInputBuffer(pbSendBuffer, cbSendLength));
  tracer.AddPassedArg("cbSendLength", std::to_string(cbSendLength));
  tracer.AddPassedArg("pioRecvPci", HexDumpPointer(pioRecvPci));
  tracer.AddPassedArg("pbRecvBuffer", HexDumpPointer(pbRecvBuffer));
  tracer.AddPassedArg("pcbRecvLength",
                      DebugDumpSCardBufferSizeInputPointer(pcbRecvLength));
  tracer.LogEntrance();

  const LONG return_code =
      pcsc_lite_->SCardTransmit(hCard, pioSendPci, pbSendBuffer, cbSendLength,
                                pioRecvPci, pbRecvBuffer, pcbRecvLength);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS ||
      return_code == SCARD_E_INSUFFICIENT_BUFFER) {
    if (pioRecvPci && return_code == SCARD_S_SUCCESS) {
      tracer.AddReturnedArg("*pioRecvPci",
                            DebugDumpSCardIoRequest(*pioRecvPci));
    }
    if (pbRecvBuffer && pcbRecvLength && return_code == SCARD_S_SUCCESS) {
      tracer.AddReturnedArg("*pbRecvBuffer", DebugDumpSCardOutputBuffer(
                                                 pbRecvBuffer, *pcbRecvLength));
    }
    if (pcbRecvLength)
      tracer.AddReturnedArg("*pcbRecvLength", std::to_string(*pcbRecvLength));
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardListReaders(SCARDCONTEXT hContext,
                                              LPCSTR mszGroups,
                                              LPSTR mszReaders,
                                              LPDWORD pcchReaders) {
  FunctionCallTracer tracer("SCardListReaders", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(hContext));
  tracer.AddPassedArg("mszGroups", DebugDumpSCardMultiString(mszGroups));
  tracer.AddPassedArg("mszReaders", HexDumpPointer(mszReaders));
  tracer.AddPassedArg("pcchReaders",
                      DebugDumpSCardBufferSizeInputPointer(pcchReaders));
  tracer.LogEntrance();
  const bool is_auto_allocation =
      pcchReaders && *pcchReaders == SCARD_AUTOALLOCATE;

  const LONG return_code = pcsc_lite_->SCardListReaders(
      hContext, mszGroups, mszReaders, pcchReaders);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS ||
      return_code == SCARD_E_INSUFFICIENT_BUFFER) {
    if (mszReaders && return_code == SCARD_S_SUCCESS) {
      tracer.AddReturnedArg("*mszReaders",
                            DebugDumpSCardOutputMultiStringBuffer(
                                mszReaders, is_auto_allocation));
    }
    if (pcchReaders)
      tracer.AddReturnedArg("*pcchReaders", std::to_string(*pcchReaders));
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardFreeMemory(SCARDCONTEXT hContext,
                                             LPCVOID pvMem) {
  FunctionCallTracer tracer("SCardFreeMemory", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(hContext));
  tracer.AddPassedArg("pvMem", HexDumpPointer(pvMem));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardFreeMemory(hContext, pvMem);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardListReaderGroups(SCARDCONTEXT hContext,
                                                   LPSTR mszGroups,
                                                   LPDWORD pcchGroups) {
  FunctionCallTracer tracer("SCardListReaderGroups", logging_prefix_,
                            log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(hContext));
  tracer.AddPassedArg("mszGroups", HexDumpPointer(mszGroups));
  tracer.AddPassedArg("pcchGroups",
                      DebugDumpSCardBufferSizeInputPointer(pcchGroups));
  tracer.LogEntrance();
  const bool is_auto_allocation =
      pcchGroups && *pcchGroups == SCARD_AUTOALLOCATE;

  const LONG return_code =
      pcsc_lite_->SCardListReaderGroups(hContext, mszGroups, pcchGroups);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  if (return_code == SCARD_S_SUCCESS ||
      return_code == SCARD_E_INSUFFICIENT_BUFFER) {
    if (mszGroups && return_code == SCARD_S_SUCCESS) {
      tracer.AddReturnedArg("*mszGroups", DebugDumpSCardOutputMultiStringBuffer(
                                              mszGroups, is_auto_allocation));
    }
    if (pcchGroups)
      tracer.AddReturnedArg("*pcchGroups", std::to_string(*pcchGroups));
  }
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardCancel(SCARDCONTEXT hContext) {
  FunctionCallTracer tracer("SCardCancel", logging_prefix_, log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(hContext));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardCancel(hContext);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

LONG PcscLiteTracingWrapper::SCardIsValidContext(SCARDCONTEXT hContext) {
  FunctionCallTracer tracer("SCardIsValidContext", logging_prefix_,
                            log_severity_);
  tracer.AddPassedArg("hContext", DebugDumpSCardContext(hContext));
  tracer.LogEntrance();

  const LONG return_code = pcsc_lite_->SCardIsValidContext(hContext);

  tracer.AddReturnValue(DebugDumpSCardReturnCode(return_code));
  tracer.LogExit();

  return return_code;
}

}  // namespace google_smart_card
