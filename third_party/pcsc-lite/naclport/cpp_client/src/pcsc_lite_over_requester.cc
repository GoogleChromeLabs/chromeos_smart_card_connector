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

#include "pcsc_lite_over_requester.h"

#include <stdint.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/multi_string.h>
#include <google_smart_card_common/requesting/remote_call_arguments_conversion.h>
#include <google_smart_card_pcsc_lite_common/scard_structs_serialization.h>

namespace google_smart_card {

namespace {

constexpr char kLoggingPrefix[] = "[PC/SC-Lite over requester] ";

// This type is used to hold temporarily the allocated memory for the PC/SC-Lite
// client API functions.
//
// The reason for using this specialization based on std::free function is that
// different PC/SC-Lite client API functions may allocate memory for structures
// of different types, but all of them should be allowed to be deallocated with
// the same single function SCardFreeMemory - that's why C++ new/delete
// operators cannot be used.
template <typename T>
using SCardUniquePtr = std::unique_ptr<T, decltype(&std::free)>;

template <typename T>
SCardUniquePtr<T> CreateSCardUniquePtr() {
  return SCardUniquePtr<T>(nullptr, &std::free);
}

// Tries to copy the data from the input range [input_begin; input_end) into the
// specified output location, which is specified in the PC/SC-Lite client API
// style: the optional output buffer, the output_size argument which points
// either to the supplied output buffer size or to the special
// SCARD_AUTOALLOCATE value.
//
// In case the output buffer size points to the SCARD_AUTOALLOCATE value, the
// output buffer is allocated by the function itself and is set to be owned by
// the SCardUniquePtr instance supplied by the allocated_buffer_holder argument.
//
// For the documentation of the corresponding behavior of the original
// PC/SC-Lite client API, refer, for instance, to
// <https://pcsclite.alioth.debian.org/api/group__API.html#gaacfec51917255b7a25b94c5104961602>.
template <typename IterT, typename T>
LONG FillOutputBufferArguments(IterT input_begin,
                               IterT input_end,
                               T* output,
                               LPDWORD output_size,
                               SCardUniquePtr<T>* allocated_buffer_holder) {
  const size_t input_size = std::distance(input_begin, input_end);

  T* target_buffer_begin = nullptr;

  if (output_size) {
    // Case a): the client passed a non-null output_size argument, which means
    // that either the client specifies the size of the output buffer, or the
    // client requests to allocate the buffer for him. In any case, after this
    // function call the output_size argument will receive the actual size of
    // the data.

    const DWORD supplied_output_size = *output_size;
    *output_size = input_size;

    if (supplied_output_size == SCARD_AUTOALLOCATE) {
      // Case a-1): the client requested to allocate the buffer for him. The
      // output argument is actually a T** in that case, and it will receive
      // pointer to the allocated buffer (so this argument is checked to be
      // non-null).
      if (!output)
        return SCARD_E_INVALID_PARAMETER;
      target_buffer_begin = reinterpret_cast<T*>(std::malloc(input_size));
      GOOGLE_SMART_CARD_CHECK(target_buffer_begin);
      allocated_buffer_holder->reset(target_buffer_begin);
      *reinterpret_cast<T**>(output) = target_buffer_begin;
    } else {
      // Case a-2): the client supplied the buffer size. If the buffer itself is
      // supplied too, then it's checked whether the supplied size is enough for
      // holding the data that will be copied in the following lines.
      if (output && supplied_output_size < input_size)
        return SCARD_E_INSUFFICIENT_BUFFER;
      target_buffer_begin = output;
    }
  } else {
    // Case b): the client didn't supply the output_size argument. If the output
    // buffer is not supplied too, then this function is essentially a no-op.
    // Otherwise (if a non-null output buffer is supplied), then this function
    // will perform an unchecked copying of the data.
    target_buffer_begin = output;
  }

  if (target_buffer_begin && input_size)
    std::memcpy(target_buffer_begin, &*input_begin, input_size);
  return SCARD_S_SUCCESS;
}

// Extracts the request result received as a response to the PC/SC-Lite client
// API request.
//
// It is assumed (and CHECKed) that the result is an array, that contains the
// PC/SC-Lite return code as the first argument (for the documentation, see
// <https://pcsclite.alioth.debian.org/api/group__ErrorCodes.html>) and the
// function output arguments as the following array items.
template <typename... Results>
LONG ExtractRequestResultsAndCode(const std::string& function_name,
                                  GenericRequestResult generic_request_result,
                                  Results*... results) {
  if (!generic_request_result.is_successful()) {
    GOOGLE_SMART_CARD_LOG_WARNING
        << kLoggingPrefix + function_name + "() failed: "
        << generic_request_result.error_message();
    return SCARD_F_INTERNAL_ERROR;
  }
  RemoteCallArgumentsExtractor extractor(
      "result of " + function_name,
      std::move(generic_request_result).TakePayload());
  LONG result_code = SCARD_F_INTERNAL_ERROR;
  extractor.Extract(&result_code);
  if (result_code == SCARD_S_SUCCESS)
    extractor.Extract(results...);
  if (!extractor.Finish())
    GOOGLE_SMART_CARD_LOG_FATAL << extractor.error_message();
  return result_code;
}

}  // namespace

PcscLiteOverRequester::PcscLiteOverRequester(
    std::unique_ptr<Requester> requester)
    : requester_(std::move(requester)),
      remote_call_adaptor_(requester_.get()) {}

PcscLiteOverRequester::~PcscLiteOverRequester() = default;

void PcscLiteOverRequester::Detach() {
  requester_->Detach();
}

LONG PcscLiteOverRequester::SCardEstablishContext(
    DWORD scope,
    LPCVOID reserved_1,
    LPCVOID reserved_2,
    LPSCARDCONTEXT s_card_context) {
  if (!s_card_context)
    return SCARD_E_INVALID_PARAMETER;
  if (reserved_1 || reserved_2) {
    // Only the NULL values of these parameters are supported by this PC/SC-Lite
    // client implementation. Anyway, PC/SC-Lite API states that these
    // parameters are not used now, so it doesn't harm much limiting to NULL's.
    return SCARD_E_INVALID_PARAMETER;
  }

  return ExtractRequestResultsAndCode(
      "SCardEstablishContext",
      remote_call_adaptor_.SyncCall("SCardEstablishContext", scope, Value(),
                                    Value()),
      s_card_context);
}

LONG PcscLiteOverRequester::SCardReleaseContext(SCARDCONTEXT s_card_context) {
  return ExtractRequestResultsAndCode(
      "SCardReleaseContext",
      remote_call_adaptor_.SyncCall("SCardReleaseContext", s_card_context));
}

LONG PcscLiteOverRequester::SCardConnect(SCARDCONTEXT s_card_context,
                                         LPCSTR reader_name,
                                         DWORD share_mode,
                                         DWORD preferred_protocols,
                                         LPSCARDHANDLE s_card_handle,
                                         LPDWORD active_protocol) {
  if (!s_card_handle || !active_protocol)
    return SCARD_E_INVALID_PARAMETER;
  if (!reader_name)
    return SCARD_E_UNKNOWN_READER;

  return ExtractRequestResultsAndCode(
      "SCardConnect",
      remote_call_adaptor_.SyncCall("SCardConnect", s_card_context, reader_name,
                                    share_mode, preferred_protocols),
      s_card_handle, active_protocol);
}

LONG PcscLiteOverRequester::SCardReconnect(SCARDHANDLE s_card_handle,
                                           DWORD share_mode,
                                           DWORD preferred_protocols,
                                           DWORD initialization_action,
                                           LPDWORD active_protocol) {
  if (!active_protocol)
    return SCARD_E_INVALID_PARAMETER;

  return ExtractRequestResultsAndCode(
      "SCardReconnect",
      remote_call_adaptor_.SyncCall("SCardReconnect", s_card_handle, share_mode,
                                    preferred_protocols, initialization_action),
      active_protocol);
}

LONG PcscLiteOverRequester::SCardDisconnect(SCARDHANDLE s_card_handle,
                                            DWORD disposition) {
  return ExtractRequestResultsAndCode(
      "SCardDisconnect", remote_call_adaptor_.SyncCall(
                             "SCardDisconnect", s_card_handle, disposition));
}

LONG PcscLiteOverRequester::SCardBeginTransaction(SCARDHANDLE s_card_handle) {
  return ExtractRequestResultsAndCode(
      "SCardBeginTransaction",
      remote_call_adaptor_.SyncCall("SCardBeginTransaction", s_card_handle));
}

LONG PcscLiteOverRequester::SCardEndTransaction(SCARDHANDLE s_card_handle,
                                                DWORD disposition_action) {
  return ExtractRequestResultsAndCode(
      "SCardEndTransaction",
      remote_call_adaptor_.SyncCall("SCardEndTransaction", s_card_handle,
                                    disposition_action));
}

LONG PcscLiteOverRequester::SCardStatus(SCARDHANDLE s_card_handle,
                                        LPSTR reader_name,
                                        LPDWORD reader_name_length,
                                        LPDWORD state,
                                        LPDWORD protocol,
                                        LPBYTE atr,
                                        LPDWORD atr_length) {
  std::string reader_name_string;
  DWORD state_copy;
  DWORD protocol_copy;
  std::vector<uint8_t> atr_vector;
  const LONG result_code = ExtractRequestResultsAndCode(
      "SCardStatus",
      remote_call_adaptor_.SyncCall("SCardStatus", s_card_handle),
      &reader_name_string, &state_copy, &protocol_copy, &atr_vector);
  GOOGLE_SMART_CARD_CHECK(result_code != SCARD_E_INSUFFICIENT_BUFFER);
  if (result_code != SCARD_S_SUCCESS)
    return result_code;

  SCardUniquePtr<char> reader_name_buffer_holder = CreateSCardUniquePtr<char>();
  const LONG reader_name_filling_result_code = FillOutputBufferArguments(
      reader_name_string.c_str(),
      reader_name_string.c_str() + reader_name_string.length() + 1, reader_name,
      reader_name_length, &reader_name_buffer_holder);

  if (state)
    *state = state_copy;

  if (protocol)
    *protocol = protocol_copy;

  SCardUniquePtr<BYTE> atr_buffer_holder = CreateSCardUniquePtr<BYTE>();
  const LONG atr_filling_result_code =
      FillOutputBufferArguments(atr_vector.begin(), atr_vector.end(), atr,
                                atr_length, &atr_buffer_holder);

  if (reader_name_filling_result_code != SCARD_S_SUCCESS)
    return reader_name_filling_result_code;
  if (atr_filling_result_code != SCARD_S_SUCCESS)
    return atr_filling_result_code;
  (void)reader_name_buffer_holder.release();
  (void)atr_buffer_holder.release();
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardGetStatusChange(
    SCARDCONTEXT s_card_context,
    DWORD timeout,
    SCARD_READERSTATE* reader_states,
    DWORD reader_states_size) {
  if (!reader_states && reader_states_size)
    return SCARD_E_INVALID_PARAMETER;

  std::vector<InboundSCardReaderState> reader_states_vector;
  for (DWORD index = 0; index < reader_states_size; ++index) {
    reader_states_vector.push_back(
        InboundSCardReaderState::FromSCardReaderState(reader_states[index]));
  }

  std::vector<OutboundSCardReaderState> returned_reader_states_vector;
  const LONG result_code = ExtractRequestResultsAndCode(
      "SCardGetStatusChange",
      remote_call_adaptor_.SyncCall("SCardGetStatusChange", s_card_context,
                                    timeout, reader_states_vector),
      &returned_reader_states_vector);
  if (result_code != SCARD_S_SUCCESS)
    return result_code;

  GOOGLE_SMART_CARD_CHECK(returned_reader_states_vector.size() ==
                          reader_states_size);
  for (DWORD index = 0; index < reader_states_size; ++index) {
    const OutboundSCardReaderState& current_returned_item =
        returned_reader_states_vector[index];
    SCARD_READERSTATE* const current_item = &reader_states[index];

    GOOGLE_SMART_CARD_CHECK(current_returned_item.reader_name ==
                            current_item->szReader);

    GOOGLE_SMART_CARD_CHECK(current_returned_item.current_state ==
                            current_item->dwCurrentState);

    current_item->dwEventState = current_returned_item.event_state;

    GOOGLE_SMART_CARD_CHECK(current_returned_item.atr.size() <= MAX_ATR_SIZE);
    current_item->cbAtr = static_cast<DWORD>(current_returned_item.atr.size());
    if (!current_returned_item.atr.empty()) {
      std::memcpy(current_item->rgbAtr, &current_returned_item.atr[0],
                  current_returned_item.atr.size());
    }
  }
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardControl(SCARDHANDLE s_card_handle,
                                         DWORD control_code,
                                         LPCVOID send_buffer,
                                         DWORD send_buffer_length,
                                         LPVOID receive_buffer,
                                         DWORD receive_buffer_length,
                                         LPDWORD bytes_returned) {
  if (send_buffer_length)
    GOOGLE_SMART_CARD_CHECK(send_buffer);
  GOOGLE_SMART_CARD_CHECK(receive_buffer);

  std::vector<uint8_t> send_buffer_vector;
  if (send_buffer_length) {
    send_buffer_vector.assign(
        static_cast<const uint8_t*>(send_buffer),
        static_cast<const uint8_t*>(send_buffer) + send_buffer_length);
  }

  std::vector<uint8_t> received_buffer_vector;
  const LONG result_code = ExtractRequestResultsAndCode(
      "SCardControl",
      remote_call_adaptor_.SyncCall("SCardControl", s_card_handle, control_code,
                                    send_buffer_vector),
      &received_buffer_vector);
  if (bytes_returned) {
    // According to PC/SC-Lite and CCID sources, zero number of written bytes is
    // reported in case of any error.
    *bytes_returned = 0;
  }
  if (result_code != SCARD_S_SUCCESS)
    return result_code;

  if (received_buffer_vector.size() > receive_buffer_length)
    return SCARD_E_INSUFFICIENT_BUFFER;
  if (!received_buffer_vector.empty()) {
    std::memcpy(receive_buffer, &received_buffer_vector[0],
                received_buffer_vector.size());
  }
  if (bytes_returned)
    *bytes_returned = received_buffer_vector.size();
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardGetAttrib(SCARDHANDLE s_card_handle,
                                           DWORD attribute_id,
                                           LPBYTE attribute_buffer,
                                           LPDWORD attribute_buffer_length) {
  std::vector<uint8_t> attribute_vector;
  const LONG result_code = ExtractRequestResultsAndCode(
      "SCardGetAttrib",
      remote_call_adaptor_.SyncCall("SCardGetAttrib", s_card_handle,
                                    attribute_id),
      &attribute_vector);
  GOOGLE_SMART_CARD_CHECK(result_code != SCARD_E_INSUFFICIENT_BUFFER);
  if (result_code != SCARD_S_SUCCESS)
    return result_code;

  SCardUniquePtr<BYTE> attribute_buffer_holder = CreateSCardUniquePtr<BYTE>();
  const LONG attribute_filling_result_code = FillOutputBufferArguments(
      attribute_vector.begin(), attribute_vector.end(), attribute_buffer,
      attribute_buffer_length, &attribute_buffer_holder);
  if (attribute_filling_result_code != SCARD_S_SUCCESS)
    return attribute_filling_result_code;
  (void)attribute_buffer_holder.release();
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardSetAttrib(SCARDHANDLE s_card_handle,
                                           DWORD attribute_id,
                                           LPCBYTE attribute_buffer,
                                           DWORD attribute_buffer_length) {
  if (!attribute_buffer || !attribute_buffer_length)
    return SCARD_E_INVALID_PARAMETER;

  const std::vector<uint8_t> attribute_buffer_vector(
      static_cast<const uint8_t*>(attribute_buffer),
      static_cast<const uint8_t*>(attribute_buffer) + attribute_buffer_length);

  return ExtractRequestResultsAndCode(
      "SCardSetAttrib",
      remote_call_adaptor_.SyncCall("SCardSetAttrib", s_card_handle,
                                    attribute_id, attribute_buffer_vector));
}

LONG PcscLiteOverRequester::SCardTransmit(
    SCARDHANDLE s_card_handle,
    const SCARD_IO_REQUEST* send_protocol_information,
    LPCBYTE send_buffer,
    DWORD send_buffer_length,
    SCARD_IO_REQUEST* receive_protocol_information,
    LPBYTE receive_buffer,
    LPDWORD receive_buffer_length) {
  if (!send_protocol_information || !send_buffer || !receive_buffer ||
      !receive_buffer_length) {
    return SCARD_E_INVALID_PARAMETER;
  }
  GOOGLE_SMART_CARD_CHECK(receive_protocol_information != SCARD_PCI_T0);
  GOOGLE_SMART_CARD_CHECK(receive_protocol_information != SCARD_PCI_T1);
  GOOGLE_SMART_CARD_CHECK(receive_protocol_information != SCARD_PCI_RAW);

  const std::vector<uint8_t> send_buffer_vector(
      send_buffer, send_buffer + send_buffer_length);
  optional<SCardIoRequest> input_receive_protocol_information;
  if (receive_protocol_information) {
    input_receive_protocol_information =
        SCardIoRequest::FromSCardIoRequest(*receive_protocol_information);
  }

  SCardIoRequest receive_protocol_information_copy;
  std::vector<uint8_t> received_buffer_vector;
  const LONG result_code = ExtractRequestResultsAndCode(
      "SCardTransmit",
      remote_call_adaptor_.SyncCall(
          "SCardTransmit", s_card_handle,
          SCardIoRequest::FromSCardIoRequest(*send_protocol_information),
          send_buffer_vector, input_receive_protocol_information),
      &receive_protocol_information_copy, &received_buffer_vector);
  if (result_code != SCARD_S_SUCCESS)
    return result_code;

  if (receive_protocol_information) {
    *receive_protocol_information =
        receive_protocol_information_copy.AsSCardIoRequest();
  }
  if (received_buffer_vector.size() > *receive_buffer_length)
    return SCARD_E_INSUFFICIENT_BUFFER;
  if (!received_buffer_vector.empty()) {
    std::memcpy(receive_buffer, &received_buffer_vector[0],
                received_buffer_vector.size());
  }
  *receive_buffer_length = received_buffer_vector.size();
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardListReaders(SCARDCONTEXT s_card_context,
                                             LPCSTR groups,
                                             LPSTR readers,
                                             LPDWORD readers_size) {
  if (groups) {
    // Only the NULL value of this parameter is supported by this PC/SC-Lite
    // client implementation. Anyway, PC/SC-Lite API states that this
    // parameter is not used now, so it doesn't harm much limiting to NULL.
    return SCARD_E_INVALID_PARAMETER;
  }
  if (!readers_size)
    return SCARD_E_INVALID_PARAMETER;
  GOOGLE_SMART_CARD_CHECK(readers_size);

  std::vector<std::string> readers_vector;
  const LONG result_code = ExtractRequestResultsAndCode(
      "SCardListReaders",
      remote_call_adaptor_.SyncCall("SCardListReaders", s_card_context,
                                    Value()),
      &readers_vector);
  GOOGLE_SMART_CARD_CHECK(result_code != SCARD_E_INSUFFICIENT_BUFFER);
  if (result_code != SCARD_S_SUCCESS)
    return result_code;

  const std::string dumped_readers = CreateMultiString(readers_vector);

  SCardUniquePtr<char> readers_buffer_holder = CreateSCardUniquePtr<char>();
  const LONG readers_filling_result_code =
      FillOutputBufferArguments(dumped_readers.begin(), dumped_readers.end(),
                                readers, readers_size, &readers_buffer_holder);
  if (readers_filling_result_code != SCARD_S_SUCCESS)
    return readers_filling_result_code;
  (void)readers_buffer_holder.release();
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardFreeMemory(SCARDCONTEXT /*s_card_context*/,
                                            LPCVOID memory) {
  std::free(const_cast<void*>(memory));
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardListReaderGroups(SCARDCONTEXT s_card_context,
                                                  LPSTR groups,
                                                  LPDWORD groups_size) {
  std::vector<std::string> groups_vector;
  const LONG result_code = ExtractRequestResultsAndCode(
      "SCardListReaderGroups",
      remote_call_adaptor_.SyncCall("SCardListReaderGroups", s_card_context),
      &groups_vector);
  GOOGLE_SMART_CARD_CHECK(result_code != SCARD_E_INSUFFICIENT_BUFFER);
  if (result_code != SCARD_S_SUCCESS)
    return result_code;

  const std::string dumped_groups = CreateMultiString(groups_vector);

  SCardUniquePtr<char> groups_buffer_holder = CreateSCardUniquePtr<char>();
  const LONG groups_filling_result_code =
      FillOutputBufferArguments(dumped_groups.begin(), dumped_groups.end(),
                                groups, groups_size, &groups_buffer_holder);
  if (groups_filling_result_code != SCARD_S_SUCCESS)
    return groups_filling_result_code;
  (void)groups_buffer_holder.release();
  return SCARD_S_SUCCESS;
}

LONG PcscLiteOverRequester::SCardCancel(SCARDCONTEXT s_card_context) {
  return ExtractRequestResultsAndCode(
      "SCardCancel",
      remote_call_adaptor_.SyncCall("SCardCancel", s_card_context));
}

LONG PcscLiteOverRequester::SCardIsValidContext(SCARDCONTEXT s_card_context) {
  return ExtractRequestResultsAndCode(
      "SCardIsValidContext",
      remote_call_adaptor_.SyncCall("SCardIsValidContext", s_card_context));
}

}  // namespace google_smart_card
