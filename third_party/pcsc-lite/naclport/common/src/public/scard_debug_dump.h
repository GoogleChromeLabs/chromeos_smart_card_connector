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

// This file contains helper functions for making debug dumps of the PC/SC-Lite
// API values (error codes, bit masks, structures, etc.).

#ifndef GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_SRC_PUBLIC_SCARD_DEBUG_DUMP_H_
#define GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_SRC_PUBLIC_SCARD_DEBUG_DUMP_H_

#include <stdint.h>

#include <string>
#include <vector>

#include <pcsclite.h>
#include <wintypes.h>

namespace google_smart_card {

std::string DebugDumpSCardReturnCode(LONG return_code);

std::string DebugDumpSCardCString(const char* value);

std::string DebugDumpSCardMultiString(const char* value);

std::string DebugDumpSCardContext(SCARDCONTEXT s_card_context);

std::string DebugDumpSCardHandle(SCARDHANDLE s_card_handle);

std::string DebugDumpSCardScope(DWORD scope);

std::string DebugDumpSCardShareMode(DWORD share_mode);

std::string DebugDumpSCardProtocol(DWORD protocol);

std::string DebugDumpSCardProtocols(DWORD protocols);

std::string DebugDumpSCardDisposition(DWORD disposition);

std::string DebugDumpSCardState(DWORD state);

std::string DebugDumpSCardEventState(DWORD event_state);

std::string DebugDumpSCardAttributeId(DWORD attribute_id);

std::string DebugDumpSCardControlCode(DWORD control_code);

std::string DebugDumpSCardIoRequest(const SCARD_IO_REQUEST& value);
std::string DebugDumpSCardIoRequest(const SCARD_IO_REQUEST* value);

std::string DebugDumpSCardInputReaderState(const SCARD_READERSTATE& value);
std::string DebugDumpSCardInputReaderStates(const SCARD_READERSTATE* begin,
                                            DWORD count);
std::string DebugDumpSCardOutputReaderState(const SCARD_READERSTATE& value);
std::string DebugDumpSCardOutputReaderStates(const SCARD_READERSTATE* begin,
                                             DWORD count);

std::string DebugDumpSCardBufferContents(const void* buffer, DWORD buffer_size);
std::string DebugDumpSCardBufferContents(const std::vector<uint8_t>& buffer);

std::string DebugDumpSCardInputBuffer(const void* buffer, DWORD buffer_size);

std::string DebugDumpSCardBufferSizeInputPointer(const DWORD* buffer_size);

std::string DebugDumpSCardOutputBuffer(const void* buffer,
                                       const DWORD* buffer_size,
                                       bool is_autoallocated);
std::string DebugDumpSCardOutputBuffer(const void* buffer, DWORD buffer_size);

std::string DebugDumpSCardOutputCStringBuffer(LPCSTR buffer,
                                              bool is_autoallocated);

std::string DebugDumpSCardOutputMultiStringBuffer(LPCSTR buffer,
                                                  bool is_autoallocated);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_THIRD_PARTY_PCSC_LITE_COMMON_SRC_PUBLIC_SCARD_DEBUG_DUMP_H_
