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

#include <google_smart_card_pcsc_lite_common/scard_debug_dump.h>

#include <reader.h>

#include <google_smart_card_common/logging/hex_dumping.h>
#include <google_smart_card_common/logging/mask_dumping.h>
#include <google_smart_card_common/multi_string.h>

namespace google_smart_card {

namespace {

struct DwordValueAndName {
  DWORD value;
  const char* name;
};

const DwordValueAndName kAttributeIdNames[] = {
    {SCARD_ATTR_ASYNC_PROTOCOL_TYPES, "SCARD_ATTR_ASYNC_PROTOCOL_TYPES"},
    {SCARD_ATTR_ATR_STRING, "SCARD_ATTR_ATR_STRING"},
    {SCARD_ATTR_CHANNEL_ID, "SCARD_ATTR_CHANNEL_ID"},
    {SCARD_ATTR_CHARACTERISTICS, "SCARD_ATTR_CHARACTERISTICS"},
    {SCARD_ATTR_CURRENT_BWT, "SCARD_ATTR_CURRENT_BWT"},
    {SCARD_ATTR_CURRENT_CLK, "SCARD_ATTR_CURRENT_CLK"},
    {SCARD_ATTR_CURRENT_CWT, "SCARD_ATTR_CURRENT_CWT"},
    {SCARD_ATTR_CURRENT_D, "SCARD_ATTR_CURRENT_D"},
    {SCARD_ATTR_CURRENT_EBC_ENCODING, "SCARD_ATTR_CURRENT_EBC_ENCODING"},
    {SCARD_ATTR_CURRENT_F, "SCARD_ATTR_CURRENT_F"},
    {SCARD_ATTR_CURRENT_IFSC, "SCARD_ATTR_CURRENT_IFSC"},
    {SCARD_ATTR_CURRENT_IFSD, "SCARD_ATTR_CURRENT_IFSD"},
    {SCARD_ATTR_CURRENT_IO_STATE, "SCARD_ATTR_CURRENT_IO_STATE"},
    {SCARD_ATTR_CURRENT_N, "SCARD_ATTR_CURRENT_N"},
    {SCARD_ATTR_CURRENT_PROTOCOL_TYPE, "SCARD_ATTR_CURRENT_PROTOCOL_TYPE"},
    {SCARD_ATTR_CURRENT_W, "SCARD_ATTR_CURRENT_W"},
    {SCARD_ATTR_DEFAULT_CLK, "SCARD_ATTR_DEFAULT_CLK"},
    {SCARD_ATTR_DEFAULT_DATA_RATE, "SCARD_ATTR_DEFAULT_DATA_RATE"},
    {SCARD_ATTR_DEVICE_FRIENDLY_NAME, "SCARD_ATTR_DEVICE_FRIENDLY_NAME"},
    {SCARD_ATTR_DEVICE_IN_USE, "SCARD_ATTR_DEVICE_IN_USE"},
    {SCARD_ATTR_DEVICE_SYSTEM_NAME, "SCARD_ATTR_DEVICE_SYSTEM_NAME"},
    {SCARD_ATTR_DEVICE_UNIT, "SCARD_ATTR_DEVICE_UNIT"},
    {SCARD_ATTR_ESC_AUTHREQUEST, "SCARD_ATTR_ESC_AUTHREQUEST"},
    {SCARD_ATTR_ESC_CANCEL, "SCARD_ATTR_ESC_CANCEL"},
    {SCARD_ATTR_ESC_RESET, "SCARD_ATTR_ESC_RESET"},
    {SCARD_ATTR_EXTENDED_BWT, "SCARD_ATTR_EXTENDED_BWT"},
    {SCARD_ATTR_ICC_INTERFACE_STATUS, "SCARD_ATTR_ICC_INTERFACE_STATUS"},
    {SCARD_ATTR_ICC_PRESENCE, "SCARD_ATTR_ICC_PRESENCE"},
    {SCARD_ATTR_ICC_TYPE_PER_ATR, "SCARD_ATTR_ICC_TYPE_PER_ATR"},
    {SCARD_ATTR_MAX_CLK, "SCARD_ATTR_MAX_CLK"},
    {SCARD_ATTR_MAX_DATA_RATE, "SCARD_ATTR_MAX_DATA_RATE"},
    {SCARD_ATTR_MAX_IFSD, "SCARD_ATTR_MAX_IFSD"},
    {SCARD_ATTR_MAXINPUT, "SCARD_ATTR_MAXINPUT"},
    {SCARD_ATTR_POWER_MGMT_SUPPORT, "SCARD_ATTR_POWER_MGMT_SUPPORT"},
    {SCARD_ATTR_SUPRESS_T1_IFS_REQUEST, "SCARD_ATTR_SUPRESS_T1_IFS_REQUEST"},
    {SCARD_ATTR_SYNC_PROTOCOL_TYPES, "SCARD_ATTR_SYNC_PROTOCOL_TYPES"},
    {SCARD_ATTR_USER_AUTH_INPUT_DEVICE, "SCARD_ATTR_USER_AUTH_INPUT_DEVICE"},
    {SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE,
     "SCARD_ATTR_USER_TO_CARD_AUTH_DEVICE"},
    {SCARD_ATTR_VENDOR_IFD_SERIAL_NO, "SCARD_ATTR_VENDOR_IFD_SERIAL_NO"},
    {SCARD_ATTR_VENDOR_IFD_TYPE, "SCARD_ATTR_VENDOR_IFD_TYPE"},
    {SCARD_ATTR_VENDOR_IFD_VERSION, "SCARD_ATTR_VENDOR_IFD_VERSION"},
    {SCARD_ATTR_VENDOR_NAME, "SCARD_ATTR_VENDOR_NAME"},
};

const DwordValueAndName kControlCodeNames[] = {
    {CM_IOCTL_GET_FEATURE_REQUEST, "CM_IOCTL_GET_FEATURE_REQUEST"},
};

template <size_t size>
std::string GetDwordValueName(
    DWORD value, const DwordValueAndName(& options)[size]) {
  for (const auto& option : options) {
    if (value == option.value)
      return option.name;
  }
  return HexDumpInteger(value);
}

}  // namespace

std::string DebugDumpSCardReturnCode(LONG return_code) {
  return std::string("\"") + pcsc_stringify_error(return_code) + "\" [" +
      HexDumpInteger(return_code) + "]";
}

std::string DebugDumpSCardCString(const char* value) {
  if (!value)
    return "<NULL string>";
  return std::string("\"") + value + "\"";
}

std::string DebugDumpSCardMultiString(const char* value) {
  if (!value)
    return "<NULL multi-string>";
  std::string result;
  for (const std::string& item : ExtractMultiStringElements(value)) {
    if (!result.empty())
      result += ", ";
    result += "\"" + item + "\"";
  }
  return "MultiString[" + result + "]";
}

std::string DebugDumpSCardContext(SCARDCONTEXT s_card_context) {
  return HexDumpInteger(s_card_context);
}

std::string DebugDumpSCardHandle(SCARDHANDLE s_card_handle) {
  return HexDumpInteger(s_card_handle);
}

std::string DebugDumpSCardScope(DWORD scope) {
  switch (scope) {
    case SCARD_SCOPE_USER:
      return "SCARD_SCOPE_USER";
    case SCARD_SCOPE_TERMINAL:
      return "SCARD_SCOPE_TERMINAL";
    case SCARD_SCOPE_SYSTEM:
      return "SCARD_SCOPE_SYSTEM";
    default:
      return HexDumpInteger(scope);
  }
}

std::string DebugDumpSCardShareMode(DWORD share_mode) {
  switch (share_mode) {
    case SCARD_SHARE_SHARED:
      return "SCARD_SHARE_SHARED";
    case SCARD_SHARE_EXCLUSIVE:
      return "SCARD_SHARE_EXCLUSIVE";
    default:
      return HexDumpInteger(share_mode);
  }
}

std::string DebugDumpSCardProtocol(DWORD protocol) {
  switch (protocol) {
    case SCARD_PROTOCOL_UNDEFINED:
      return "SCARD_PROTOCOL_UNDEFINED";
    case SCARD_PROTOCOL_T0:
      return "SCARD_PROTOCOL_T0";
    case SCARD_PROTOCOL_T1:
      return "SCARD_PROTOCOL_T1";
    case SCARD_PROTOCOL_RAW:
      return "SCARD_PROTOCOL_RAW";
    case SCARD_PROTOCOL_T15:
      return "SCARD_PROTOCOL_T15";
    case SCARD_PROTOCOL_ANY:
      return "SCARD_PROTOCOL_ANY";
    default:
      return HexDumpInteger(protocol);
  }
}

std::string DebugDumpSCardProtocols(DWORD protocols) {
  return DumpMask(protocols, {
      {SCARD_PROTOCOL_T0, "SCARD_PROTOCOL_T0"},
      {SCARD_PROTOCOL_T1, "SCARD_PROTOCOL_T1"},
      {SCARD_PROTOCOL_RAW, "SCARD_PROTOCOL_RAW"},
      {SCARD_PROTOCOL_T15, "SCARD_PROTOCOL_T15"}});
}

std::string DebugDumpSCardDisposition(DWORD disposition) {
  switch (disposition) {
    case SCARD_LEAVE_CARD:
      return "SCARD_LEAVE_CARD";
    case SCARD_RESET_CARD:
      return "SCARD_RESET_CARD";
    case SCARD_UNPOWER_CARD:
      return "SCARD_UNPOWER_CARD";
    case SCARD_EJECT_CARD:
      return "SCARD_EJECT_CARD";
    default:
      return HexDumpInteger(disposition);
  }
}

std::string DebugDumpSCardState(DWORD state) {
  std::string suffix;
  const DWORD kEventCountMask = 0xFFFF0000;
  const int kEventCountMaskShift = 16;
  const DWORD event_count = (state & kEventCountMask) >> kEventCountMaskShift;
  state &= ~kEventCountMask;
  if (event_count)
    suffix = " with eventCount=" + std::to_string(event_count);

  return DumpMask(state, {
      {SCARD_ABSENT, "SCARD_ABSENT"},
      {SCARD_PRESENT, "SCARD_PRESENT"},
      {SCARD_SWALLOWED, "SCARD_SWALLOWED"},
      {SCARD_POWERED, "SCARD_POWERED"},
      {SCARD_NEGOTIABLE, "SCARD_NEGOTIABLE"},
      {SCARD_SPECIFIC, "SCARD_SPECIFIC"}}) + suffix;
}

std::string DebugDumpSCardEventState(DWORD event_state) {
  std::string suffix;
  const DWORD kEventCountMask = 0xFFFF0000;
  const int kEventCountMaskShift = 16;
  const DWORD event_count =
      (event_state & kEventCountMask) >> kEventCountMaskShift;
  event_state &= ~kEventCountMask;
  if (event_count)
    suffix = " with eventCount=" + std::to_string(event_count);

  if (!event_state)
    return "SCARD_STATE_UNAWARE" + suffix;
  return DumpMask(event_state, {
      {SCARD_STATE_IGNORE, "SCARD_STATE_IGNORE"},
      {SCARD_STATE_CHANGED, "SCARD_STATE_CHANGED"},
      {SCARD_STATE_UNKNOWN, "SCARD_STATE_UNKNOWN"},
      {SCARD_STATE_UNAVAILABLE, "SCARD_STATE_UNAVAILABLE"},
      {SCARD_STATE_EMPTY, "SCARD_STATE_EMPTY"},
      {SCARD_STATE_PRESENT, "SCARD_STATE_PRESENT"},
      {SCARD_STATE_ATRMATCH, "SCARD_STATE_ATRMATCH"},
      {SCARD_STATE_EXCLUSIVE, "SCARD_STATE_EXCLUSIVE"},
      {SCARD_STATE_INUSE, "SCARD_STATE_INUSE"},
      {SCARD_STATE_MUTE, "SCARD_STATE_MUTE"},
      {SCARD_STATE_UNPOWERED, "SCARD_STATE_UNPOWERED"}}) + suffix;
}

std::string DebugDumpSCardAttributeId(DWORD attribute_id) {
  return GetDwordValueName(attribute_id, kAttributeIdNames);
}

std::string DebugDumpSCardControlCode(DWORD control_code) {
  return GetDwordValueName(control_code, kControlCodeNames);
}

std::string DebugDumpSCardIoRequest(const SCARD_IO_REQUEST& value) {
  if (&value == SCARD_PCI_T0)
    return "SCARD_PCI_T0";
  if (&value == SCARD_PCI_T1)
    return "SCARD_PCI_T1";
  if (&value == SCARD_PCI_RAW)
    return "SCARD_PCI_RAW";
  return "SCARD_IO_REQUEST(dwProtocol=" +
         DebugDumpSCardProtocol(value.dwProtocol) + ")";
}

std::string DebugDumpSCardIoRequest(const SCARD_IO_REQUEST* value) {
  if (!value)
    return "NULL";
  return HexDumpPointer(value) + "(" + DebugDumpSCardIoRequest(*value) + ")";
}

std::string DebugDumpSCardInputReaderState(const SCARD_READERSTATE& value) {
  return "SCARD_READERSTATE(szReader=" + DebugDumpSCardCString(value.szReader) +
      ", pvUserData=" + HexDumpPointer(value.pvUserData) + ", dwCurrentState=" +
      DebugDumpSCardEventState(value.dwCurrentState) + ")";
}

std::string DebugDumpSCardInputReaderStates(
    const SCARD_READERSTATE* begin, DWORD count) {
  if (!begin)
    return "NULL";
  std::string result;
  for (DWORD index = 0; index < count; ++index) {
    if (!result.empty())
      result += ", ";
    result += DebugDumpSCardInputReaderState(begin[index]);
  }
  return HexDumpPointer(begin) + "([" + result + "])";
}

std::string DebugDumpSCardOutputReaderState(const SCARD_READERSTATE& value) {
  return "SCARD_READERSTATE(szReader=" + DebugDumpSCardCString(value.szReader) +
      ", pvUserData=" + HexDumpPointer(value.pvUserData) + ", dwCurrentState=" +
      DebugDumpSCardEventState(value.dwCurrentState) + ", dwEventState=" +
      DebugDumpSCardEventState(value.dwEventState) + ", cbAtr=" +
      std::to_string(value.cbAtr) + ", rgbAtr=<" +
      HexDumpBytes(value.rgbAtr, value.cbAtr) + ">)";
}

std::string DebugDumpSCardOutputReaderStates(
    const SCARD_READERSTATE* begin, DWORD count) {
  if (!begin)
    return "NULL";
  std::string result;
  for (DWORD index = 0; index < count; ++index) {
    if (!result.empty())
      result += ", ";
    result += DebugDumpSCardOutputReaderState(begin[index]);
  }
  return HexDumpPointer(begin) + "([" + result + "])";
}

std::string DebugDumpSCardInputBuffer(const void* buffer, DWORD buffer_size) {
  if (!buffer)
    return "NULL";
  return HexDumpPointer(buffer) + "(<" + HexDumpBytes(buffer, buffer_size) +
         ">)";
}

std::string DebugDumpSCardBufferSizeInputPointer(const DWORD* buffer_size) {
  if (!buffer_size)
    return "NULL";
  const std::string dumped_value = *buffer_size == SCARD_AUTOALLOCATE ?
      "SCARD_AUTOALLOCATE" : std::to_string(*buffer_size);
  return HexDumpPointer(buffer_size) + "(" + dumped_value + ")";
}

std::string DebugDumpSCardOutputBuffer(
    const void* buffer, const DWORD* buffer_size, bool is_autoallocated) {
  const void* const contents = is_autoallocated ?
      *reinterpret_cast<const void* const*>(buffer) : buffer;
  const std::string dumped_value = "<" +
      (buffer_size ? HexDumpBytes(buffer, *buffer_size) :
           "DATA OF UNKNOWN LENGTH") + ">";
  return is_autoallocated ? HexDumpPointer(buffer) + "(" + dumped_value + ")" :
      dumped_value;
}

std::string DebugDumpSCardOutputBuffer(const void* buffer, DWORD buffer_size) {
  return "<" + HexDumpBytes(buffer, buffer_size) + ">";
}

std::string DebugDumpSCardOutputCStringBuffer(
    LPCSTR buffer, bool is_autoallocated) {
  const LPCSTR string_value = is_autoallocated ?
      *reinterpret_cast<const LPCSTR*>(buffer) : buffer;
  const std::string dumped_value = DebugDumpSCardCString(string_value);
  return is_autoallocated ?
      HexDumpPointer(string_value) + "(" + dumped_value + ")" : dumped_value;
}

std::string DebugDumpSCardOutputMultiStringBuffer(
    LPCSTR buffer, bool is_autoallocated) {
  const LPCSTR multi_string_value = is_autoallocated ?
      *reinterpret_cast<const LPCSTR*>(buffer) : buffer;
  const std::string dumped_value = DebugDumpSCardMultiString(
      multi_string_value);
  return is_autoallocated ?
      HexDumpPointer(multi_string_value) + "(" + dumped_value + ")" :
      dumped_value;
}

}  // namespace google_smart_card
