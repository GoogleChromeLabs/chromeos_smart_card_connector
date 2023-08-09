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

#include <google_smart_card_pcsc_lite_cpp_demo/demo.h>

#include <stdint.h>

#include <chrono>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <pcsclite.h>
#include <reader.h>
#include <winscard.h>
#include <wintypes.h>

#include "common/cpp/src/public/logging/hex_dumping.h"
#include "common/cpp/src/public/logging/logging.h"
#include "common/cpp/src/public/multi_string.h"
#include "third_party/pcsc-lite/naclport/common/src/google_smart_card_pcsc_lite_common/scard_debug_dump.h"

namespace google_smart_card {

namespace {

constexpr int kWaitingTimeoutSeconds = 10;

constexpr char kSpecialReaderName[] = "\\\\?PnP?\\Notification";

constexpr DWORD kAttrIds[] = {
    SCARD_ATTR_ASYNC_PROTOCOL_TYPES,
    SCARD_ATTR_ATR_STRING,
    SCARD_ATTR_CHANNEL_ID,
    SCARD_ATTR_CHARACTERISTICS,
    SCARD_ATTR_CURRENT_BWT,
    SCARD_ATTR_CURRENT_CLK,
    SCARD_ATTR_CURRENT_CWT,
    SCARD_ATTR_CURRENT_D,
    SCARD_ATTR_CURRENT_EBC_ENCODING,
    SCARD_ATTR_CURRENT_F,
    SCARD_ATTR_CURRENT_IFSC,
    SCARD_ATTR_CURRENT_IFSD,
    SCARD_ATTR_CURRENT_IO_STATE,
    SCARD_ATTR_CURRENT_N,
    SCARD_ATTR_CURRENT_PROTOCOL_TYPE,
    SCARD_ATTR_CURRENT_W,
    SCARD_ATTR_DEFAULT_CLK,
    SCARD_ATTR_DEFAULT_DATA_RATE,
    SCARD_ATTR_DEVICE_FRIENDLY_NAME,
    SCARD_ATTR_DEVICE_IN_USE,
    SCARD_ATTR_DEVICE_SYSTEM_NAME,
    SCARD_ATTR_DEVICE_UNIT,
    SCARD_ATTR_ESC_AUTHREQUEST,
    SCARD_ATTR_ESC_CANCEL,
    SCARD_ATTR_ESC_RESET,
    SCARD_ATTR_EXTENDED_BWT,
    SCARD_ATTR_ICC_INTERFACE_STATUS,
    SCARD_ATTR_ICC_PRESENCE,
    SCARD_ATTR_ICC_TYPE_PER_ATR,
    SCARD_ATTR_MAX_CLK,
    SCARD_ATTR_MAX_DATA_RATE,
    SCARD_ATTR_MAX_IFSD,
    SCARD_ATTR_MAXINPUT,
    SCARD_ATTR_POWER_MGMT_SUPPORT,
    SCARD_ATTR_SUPRESS_T1_IFS_REQUEST,
    SCARD_ATTR_SYNC_PROTOCOL_TYPES,
    SCARD_ATTR_USER_AUTH_INPUT_DEVICE,
    SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE,
    SCARD_ATTR_VENDOR_IFD_SERIAL_NO,
    SCARD_ATTR_VENDOR_IFD_TYPE,
    SCARD_ATTR_VENDOR_IFD_VERSION,
    SCARD_ATTR_VENDOR_NAME,
};

constexpr char kLoggingPrefix[] = "[PC/SC-Lite DEMO]";

std::string FormatSCardErrorMessage(LONG return_code) {
  GOOGLE_SMART_CARD_CHECK(return_code != SCARD_S_SUCCESS);
  std::ostringstream stream;
  stream << "failed with the following error: "
         << DebugDumpSCardReturnCode(return_code) << ".";
  return stream.str();
}

size_t GetMultiStringLength(LPSTR multi_string) {
  size_t result = 0;
  for (const std::string& item : ExtractMultiStringElements(multi_string))
    result += item.length() + 1;
  if (!result)
    return 2;
  return result + 1;
}

bool DoPcscLiteContextEstablishing(SCARDCONTEXT* s_card_context) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardEstablishContext()...";
  const LONG return_code = SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr,
                                                 nullptr, s_card_context);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }
  if (!s_card_context) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "zero context.";
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    returned context="
                             << DebugDumpSCardContext(*s_card_context) << ".";
  return true;
}

bool DoPcscLiteContextValidation(SCARDCONTEXT s_card_context) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardIsValidContext()...";
  const LONG return_code = SCardIsValidContext(s_card_context);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    success.";
  return true;
}

bool DoPcscLiteInvalidContextValidation(SCARDCONTEXT s_card_context) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardIsValidContext() with invalid context...";
  const LONG return_code = SCardIsValidContext(s_card_context + 1);
  if (return_code != SCARD_E_INVALID_HANDLE) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: instead "
                                << "of \"invalid context\" error, returned "
                                << DebugDumpSCardReturnCode(return_code) << ".";
    return false;
  }
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    successfully rejected.";
  return true;
}

bool DoPcscLiteReadersChangeWaiting(SCARDCONTEXT s_card_context) {
  const LPVOID kUserData = reinterpret_cast<LPVOID>(0xDEADBEEF);  // NOLINT

  SCARD_READERSTATE reader_states[1];
  std::memset(&reader_states, 0, sizeof reader_states);
  reader_states[0].szReader = kSpecialReaderName;
  reader_states[0].pvUserData = kUserData;  // NOLINT
  GOOGLE_SMART_CARD_LOG_INFO
      << kLoggingPrefix << "  Calling "
      << "SCardGetStatusChange() for waiting for readers change for "
      << kWaitingTimeoutSeconds << " seconds...";
  const LONG return_code = SCardGetStatusChange(
      s_card_context, kWaitingTimeoutSeconds * 1000, reader_states, 1);
  if (return_code == SCARD_E_TIMEOUT) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    no readers change "
                               << "events were caught.";
    return true;
  }
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  if (std::string(reader_states[0].szReader) != kSpecialReaderName) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "wrong reader name.";
    return false;
  }
  if (reader_states[0].pvUserData != kUserData) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "wrong user data.";
    return false;
  }
  if (!(reader_states[0].dwEventState & SCARD_STATE_CHANGED)) {
    GOOGLE_SMART_CARD_LOG_ERROR
        << kLoggingPrefix << "    failed: returned "
        << "current state mask ("
        << DebugDumpSCardEventState(reader_states[0].dwEventState) << ") "
        << "without SCARD_STATE_CHANGED bit.";
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    caught readers "
                             << "change event.";

  return true;
}

bool DoPcscLiteReaderGroupsListing(SCARDCONTEXT s_card_context) {
  LONG return_code;

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardListReaderGroups()...";
  LPSTR groups = nullptr;
  DWORD groups_buffer_length = SCARD_AUTOALLOCATE;
  return_code = SCardListReaderGroups(
      s_card_context, reinterpret_cast<LPSTR>(&groups), &groups_buffer_length);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  bool result = true;

  if (result && !groups) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "null groups multi string.";
    result = false;
  }
  if (result && groups_buffer_length == SCARD_AUTOALLOCATE) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "no groups multi string length.";
    result = false;
  }
  if (result && GetMultiStringLength(groups) != groups_buffer_length) {
    GOOGLE_SMART_CARD_LOG_ERROR
        << kLoggingPrefix << "    failed: returned "
        << "wrong multi string length: " << groups_buffer_length << ", while "
        << "multi string itself has length " << GetMultiStringLength(groups)
        << ".";
    result = false;
  }
  if (result && !*groups) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: no reader "
                                << "groups were returned.";
    result = false;
  }

  if (result) {
    GOOGLE_SMART_CARD_LOG_INFO
        << kLoggingPrefix << "    returned reader "
        << "groups: " << DebugDumpSCardMultiString(groups) << ".";
  }

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "  Calling "
                              << "SCardFreeMemory()...";
  return_code = SCardFreeMemory(s_card_context, groups);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Called "
                               << "SCardFreeMemory()...";
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    result = false;
  }

  return result;
}

bool DoPcscLiteReadersListing(SCARDCONTEXT s_card_context,
                              std::string* reader_name) {
  LONG return_code;

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardListReaders()...";
  LPSTR readers = nullptr;
  DWORD readers_buffer_length = SCARD_AUTOALLOCATE;
  return_code = SCardListReaders(s_card_context, nullptr,
                                 reinterpret_cast<LPSTR>(&readers),
                                 &readers_buffer_length);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  bool result = true;

  if (result && !readers) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "null readers multi string.";
    result = false;
  }
  if (result && readers_buffer_length == SCARD_AUTOALLOCATE) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "no readers multi string length.";
    result = false;
  }
  if (result && GetMultiStringLength(readers) != readers_buffer_length) {
    GOOGLE_SMART_CARD_LOG_ERROR
        << kLoggingPrefix << "    failed: returned "
        << "wrong multi string length: " << readers_buffer_length << ", while "
        << "multi string itself has length " << GetMultiStringLength(readers)
        << ".";
    result = false;
  }
  if (result && !*readers) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: no "
                                << "readers were returned.";
    result = false;
  }

  if (result) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    returned readers: "
                               << DebugDumpSCardMultiString(readers) << ".";
    *reader_name = readers;
  }

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "  Calling "
                              << "SCardFreeMemory()...";
  return_code = SCardFreeMemory(s_card_context, readers);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Called "
                               << "SCardFreeMemory()...";
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    result = false;
  }

  return result;
}

bool DoPcscLiteCardRemovalWaiting(SCARDCONTEXT s_card_context,
                                  const std::string& reader_name) {
  SCARD_READERSTATE reader_states[1];
  std::memset(&reader_states, 0, sizeof reader_states);
  reader_states[0].szReader = reader_name.c_str();
  reader_states[0].dwCurrentState = SCARD_STATE_PRESENT;
  GOOGLE_SMART_CARD_LOG_INFO
      << kLoggingPrefix << "  Calling "
      << "SCardGetStatusChange() for waiting for card removal for "
      << kWaitingTimeoutSeconds << " seconds...";
  const LONG return_code = SCardGetStatusChange(
      s_card_context, kWaitingTimeoutSeconds * 1000, reader_states, 1);
  if (return_code == SCARD_E_TIMEOUT) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    no card removal "
                               << "events were caught.";
    return true;
  }
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  if (!(reader_states[0].dwEventState & SCARD_STATE_EMPTY)) {
    GOOGLE_SMART_CARD_LOG_ERROR
        << kLoggingPrefix << "    failed: returned "
        << "event state mask ("
        << DebugDumpSCardEventState(reader_states[0].dwEventState) << ") "
        << "without SCARD_STATE_EMPTY bit.";
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    caught card removal "
                             << "event";

  return true;
}

bool DoPcscLiteCardInsertionWaiting(SCARDCONTEXT s_card_context,
                                    const std::string& reader_name) {
  SCARD_READERSTATE reader_states[1];
  std::memset(&reader_states, 0, sizeof reader_states);
  reader_states[0].szReader = reader_name.c_str();
  reader_states[0].dwCurrentState = SCARD_STATE_EMPTY;
  GOOGLE_SMART_CARD_LOG_INFO
      << kLoggingPrefix << "  Calling "
      << "SCardGetStatusChange() for waiting for card insertion for "
      << kWaitingTimeoutSeconds << " seconds...";
  const LONG return_code = SCardGetStatusChange(
      s_card_context, kWaitingTimeoutSeconds * 1000, reader_states, 1);
  if (return_code == SCARD_E_TIMEOUT) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    no card insertion "
                               << "events were caught.";
    return true;
  }
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  if (!(reader_states[0].dwEventState & SCARD_STATE_PRESENT)) {
    GOOGLE_SMART_CARD_LOG_ERROR
        << kLoggingPrefix << "    failed: returned "
        << "event state mask ("
        << DebugDumpSCardEventState(reader_states[0].dwEventState) << ") "
        << "without SCARD_STATE_PRESENT bit.";
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    caught card "
                             << "insertion event.";

  return true;
}

bool DoPcscLiteWaitingAndCancellation(SCARDCONTEXT s_card_context) {
  std::thread([s_card_context] {
    // Wait until SCardGetStatusChange in parallel thread is called. This is not
    // a 100%-correct solution, but should work fine enough for demo purposes.
    std::this_thread::sleep_for(
        std::chrono::milliseconds(kWaitingTimeoutSeconds * 1000 / 10));

    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                               << "SCardCancel()...";
    const LONG return_code = SCardCancel(s_card_context);
    if (return_code == SCARD_S_SUCCESS) {
      GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    successfully "
                                 << "canceled";
    } else {
      GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    returned "
                                 << DebugDumpSCardReturnCode(return_code)
                                 << ".";
    }
  }).detach();

  SCARD_READERSTATE reader_states[1];
  std::memset(&reader_states, 0, sizeof reader_states);
  reader_states[0].szReader = kSpecialReaderName;
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardGetStatusChange()...";

  const LONG return_code = SCardGetStatusChange(
      s_card_context, kWaitingTimeoutSeconds * 1000, reader_states, 1);

  // Wait until the parallel thread with SCardCancel() (hopefully) finishes -
  // just for having the fancy log messages order.
  std::this_thread::sleep_for(
      std::chrono::milliseconds(kWaitingTimeoutSeconds * 1000 / 10));

  if (return_code != SCARD_E_CANCELLED) {
    GOOGLE_SMART_CARD_LOG_INFO
        << kLoggingPrefix << "    failed: expected "
        << "the waiting to return with \"cancelled\" state, instead returned "
        << "with " << DebugDumpSCardReturnCode(return_code) << ".";
    return true;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    caught waiting "
                             << "cancellation.";

  return true;
}

bool DoPcscLiteConnect(SCARDCONTEXT s_card_context,
                       const std::string& reader_name,
                       SCARDHANDLE* s_card_handle,
                       DWORD* active_protocol) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling SCardConnect() "
                             << "for connecting to the \"" << reader_name
                             << "\" reader...";
  const LONG return_code =
      SCardConnect(s_card_context, reader_name.c_str(), SCARD_SHARE_SHARED,
                   SCARD_PROTOCOL_ANY, s_card_handle, active_protocol);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO
      << kLoggingPrefix
      << "    returned card_handle=" << HexDumpInteger(*s_card_handle)
      << ", active_protocol=" << DebugDumpSCardProtocol(*active_protocol)
      << ".";
  return true;
}

bool DoPcscLiteReconnect(SCARDHANDLE s_card_handle) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardReconnect()...";
  DWORD active_protocol;
  const LONG return_code =
      SCardReconnect(s_card_handle, SCARD_SHARE_SHARED, SCARD_PROTOCOL_ANY,
                     SCARD_LEAVE_CARD, &active_protocol);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    succeeded.";
  return true;
}

bool DoPcscLiteGetStatus(SCARDCONTEXT s_card_context,
                         SCARDHANDLE s_card_handle) {
  LONG return_code;

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling SCardStatus()...";
  LPSTR reader = nullptr;
  DWORD reader_buffer_length = SCARD_AUTOALLOCATE;
  DWORD state = 0;
  DWORD protocol = 0;
  LPBYTE atr = nullptr;
  DWORD atr_len = SCARD_AUTOALLOCATE;
  return_code = SCardStatus(s_card_handle, reinterpret_cast<LPSTR>(&reader),
                            &reader_buffer_length, &state, &protocol,
                            reinterpret_cast<LPBYTE>(&atr), &atr_len);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  bool result = true;

  if (result && !reader) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "null reader string.";
    result = false;
  }
  if (result && reader_buffer_length == SCARD_AUTOALLOCATE) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "no reader string length.";
    result = false;
  }
  if (result && reader_buffer_length != std::strlen(reader) + 1) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "wrong reader string length.";
    result = false;
  }
  if (result && !atr) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "null atr buffer.";
    result = false;
  }
  if (result && atr_len == SCARD_AUTOALLOCATE) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: returned "
                                << "no atr buffer length.";
    result = false;
  }

  if (result) {
    GOOGLE_SMART_CARD_LOG_INFO
        << kLoggingPrefix << "    returned name=\"" << reader
        << "\", state=" << DebugDumpSCardState(state)
        << ", protocol=" << DebugDumpSCardProtocol(protocol) << ".";
  }

  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "  Calling "
                              << "SCardFreeMemory()...";
  return_code = SCardFreeMemory(s_card_context, reader);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Called "
                               << "SCardFreeMemory()...";
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    result = false;
  }
  GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "  Calling "
                              << "SCardFreeMemory()...";
  return_code = SCardFreeMemory(s_card_context, atr);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Called "
                               << "SCardFreeMemory()...";
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    result = false;
  }

  return result;
}

bool DoPcscLiteGetAttrs(SCARDCONTEXT s_card_context,
                        SCARDHANDLE s_card_handle) {
  LONG return_code;

  for (DWORD attr_id : kAttrIds) {
    LPBYTE value = nullptr;
    DWORD value_length = SCARD_AUTOALLOCATE;
    GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "  Calling "
                                << "SCardGetAttrib() for attribute \""
                                << DebugDumpSCardAttributeId(attr_id)
                                << "\"...";
    return_code =
        SCardGetAttrib(s_card_handle, attr_id, reinterpret_cast<LPBYTE>(&value),
                       &value_length);
    if (return_code == SCARD_S_SUCCESS) {
      GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Called "
                                 << "SCardGetAttrib() for attribute \""
                                 << DebugDumpSCardAttributeId(attr_id)
                                 << "\"...";

      if (!value) {
        GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    failed: "
                                    << "returned null value";
        return false;
      }

      GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    returned <"
                                 << HexDumpBytes(value, value_length) << ">.";

      GOOGLE_SMART_CARD_LOG_DEBUG << kLoggingPrefix << "  Calling "
                                  << "SCardFreeMemory()...";
      return_code = SCardFreeMemory(s_card_context, value);
      if (return_code != SCARD_S_SUCCESS) {
        GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Called "
                                   << "SCardFreeMemory()...";
        GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                    << FormatSCardErrorMessage(return_code);
        return false;
      }
    }
  }
  return true;
}

bool DoPcscLiteSetAttr(SCARDHANDLE s_card_handle) {
  const std::string kValue = "Test";
  GOOGLE_SMART_CARD_LOG_INFO
      << kLoggingPrefix << "  Calling "
      << "SCardSetAttrib() for attrib \"SCARD_ATTR_DEVICE_FRIENDLY_NAME_A\"...";
  LONG return_code = SCardSetAttrib(
      s_card_handle, SCARD_ATTR_DEVICE_FRIENDLY_NAME_A,
      reinterpret_cast<LPCBYTE>(kValue.c_str()), kValue.length());
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    set unsuccessfully "
                               << "with error "
                               << DebugDumpSCardReturnCode(return_code) << ".";
    return true;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    succeeded.";
  return true;
}

bool DoPcscLiteBeginTransaction(SCARDHANDLE s_card_handle) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardBeginTransaction()...";
  LONG return_code = SCardBeginTransaction(s_card_handle);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    succeeded.";
  return true;
}

bool DoPcscLiteSendControlCommand(SCARDHANDLE s_card_handle) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling SCardControl() "
                             << "with CM_IOCTL_GET_FEATURE_REQUEST command...";
  unsigned char buffer[MAX_BUFFER_SIZE_EXTENDED];
  DWORD bytes_returned = 0;
  LONG return_code =
      SCardControl(s_card_handle, CM_IOCTL_GET_FEATURE_REQUEST, nullptr, 0,
                   buffer, MAX_BUFFER_SIZE_EXTENDED, &bytes_returned);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    returned <"
                             << HexDumpBytes(buffer, bytes_returned) << ">.";
  return true;
}

bool DoPcscLiteSendTransmitCommand(SCARDHANDLE s_card_handle,
                                   DWORD active_protocol) {
  const std::vector<uint8_t> kListDirApdu{0x00, 0xA4, 0x00, 0x00,
                                          0x02, 0x3F, 0x00, 0x00};
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardTransmit() with \"list dir\" APDU <"
                             << HexDumpBytes(kListDirApdu) << ">...";
  SCARD_IO_REQUEST received_protocol;
  received_protocol.dwProtocol = SCARD_PROTOCOL_ANY;
  received_protocol.cbPciLength = sizeof(SCARD_IO_REQUEST);
  unsigned char buffer[MAX_BUFFER_SIZE_EXTENDED];
  DWORD bytes_returned = MAX_BUFFER_SIZE_EXTENDED;
  LONG return_code = SCardTransmit(
      s_card_handle,
      active_protocol == SCARD_PROTOCOL_T0 ? SCARD_PCI_T0 : SCARD_PCI_T1,
      &kListDirApdu[0], kListDirApdu.size(), &received_protocol, buffer,
      &bytes_returned);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    returned <"
                             << HexDumpBytes(buffer, bytes_returned) << ">.";
  return true;
}

bool DoPcscLiteEndTransaction(SCARDHANDLE s_card_handle) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardEndTransaction()...";
  LONG return_code = SCardEndTransaction(s_card_handle, SCARD_LEAVE_CARD);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    succeeded.";
  return true;
}

bool DoPcscLiteDisconnect(SCARDHANDLE s_card_handle) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardDisconnect()...";
  LONG return_code = SCardDisconnect(s_card_handle, SCARD_LEAVE_CARD);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }

  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    succeeded.";
  return true;
}

bool DoPcscLiteContextReleasing(SCARDCONTEXT s_card_context) {
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "  Calling "
                             << "SCardReleaseContext()...";
  LONG return_code = SCardReleaseContext(s_card_context);
  if (return_code != SCARD_S_SUCCESS) {
    GOOGLE_SMART_CARD_LOG_ERROR << kLoggingPrefix << "    "
                                << FormatSCardErrorMessage(return_code);
    return false;
  }
  GOOGLE_SMART_CARD_LOG_INFO << kLoggingPrefix << "    succeeded.";
  return true;
}

}  // namespace

bool ExecutePcscLiteCppDemo() {
  bool result = true;

  SCARDCONTEXT s_card_context;
  result = result && DoPcscLiteContextEstablishing(&s_card_context);
  if (result) {
    result = result && DoPcscLiteContextValidation(s_card_context);
    result = result && DoPcscLiteInvalidContextValidation(s_card_context);
    result = result && DoPcscLiteReadersChangeWaiting(s_card_context);
    result = result && DoPcscLiteReaderGroupsListing(s_card_context);

    std::string reader_name;
    result = result && DoPcscLiteReadersListing(s_card_context, &reader_name);
    if (result) {
      result =
          result && DoPcscLiteCardRemovalWaiting(s_card_context, reader_name);
      result =
          result && DoPcscLiteCardInsertionWaiting(s_card_context, reader_name);
      result = result && DoPcscLiteWaitingAndCancellation(s_card_context);

      SCARDHANDLE s_card_handle;
      DWORD active_protocol;
      result = result && DoPcscLiteConnect(s_card_context, reader_name,
                                           &s_card_handle, &active_protocol);
      if (result) {
        result = result && DoPcscLiteReconnect(s_card_handle);
        result = result && DoPcscLiteGetStatus(s_card_context, s_card_handle);
        result = result && DoPcscLiteGetAttrs(s_card_context, s_card_handle);
        result = result && DoPcscLiteSetAttr(s_card_handle);
        result = result && DoPcscLiteBeginTransaction(s_card_handle);
        if (result) {
          result = result && DoPcscLiteSendControlCommand(s_card_handle);
          result = result && DoPcscLiteSendTransmitCommand(s_card_handle,
                                                           active_protocol);

          result = DoPcscLiteEndTransaction(s_card_handle) && result;
        }

        result = DoPcscLiteDisconnect(s_card_handle) && result;
      }
    }

    result = DoPcscLiteContextReleasing(s_card_context) && result;
  }

  return result;
}

}  // namespace google_smart_card
