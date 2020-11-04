// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file contains functions for creating a hexadecimal representation.

#ifndef GOOGLE_SMART_CARD_COMMON_LOGGING_HEX_DUMP_H_
#define GOOGLE_SMART_CARD_COMMON_LOGGING_HEX_DUMP_H_

#include <stdint.h>

#include <string>
#include <type_traits>
#include <vector>

namespace google_smart_card {

//
// Returns the given byte value in the "0xNN" hexadecimal format.
//

std::string HexDumpByte(int8_t value);

std::string HexDumpByte(uint8_t value);

//
// Returns the given 2-byte value in the "0xNNNN" hexadecimal format.
//

std::string HexDumpDoublet(int16_t value);

std::string HexDumpDoublet(uint16_t value);

//
// Returns the given 4-byte value in the "0xNNNNNNNN" hexadecimal format.
//

std::string HexDumpQuadlet(int32_t value);

std::string HexDumpQuadlet(uint32_t value);

//
// Returns the given 8-byte value in the "0xNNNNNNNNNNNNNNNN" hexadecimal
// format.
//

std::string HexDumpOctlet(int64_t value);

std::string HexDumpOctlet(uint64_t value);

// Returns the pointer address value in the hexadecimal format.
//
// The actual number of digits depends on the platform size of the pointers.
std::string HexDumpPointer(const void* value);

//
// Returns the given value in the "0x..." format (the result string length is
// determined by the number bit length).
//

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(int8_t) &&
                                   std::is_signed<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpByte(static_cast<int8_t>(value));
}

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(uint8_t) &&
                                   std::is_unsigned<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpByte(static_cast<uint8_t>(value));
}

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(int16_t) &&
                                   std::is_signed<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpDoublet(static_cast<int16_t>(value));
}

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(uint16_t) &&
                                   std::is_unsigned<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpDoublet(static_cast<uint16_t>(value));
}

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(int32_t) &&
                                   std::is_signed<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpQuadlet(static_cast<int32_t>(value));
}

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(uint32_t) &&
                                   std::is_unsigned<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpQuadlet(static_cast<uint32_t>(value));
}

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(int64_t) &&
                                   std::is_signed<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpOctlet(static_cast<int64_t>(value));
}

template <typename T>
inline typename std::enable_if<sizeof(T) == sizeof(uint64_t) &&
                                   std::is_unsigned<T>::value,
                               std::string>::type
HexDumpInteger(T value) {
  return HexDumpOctlet(static_cast<uint64_t>(value));
}

//
// Returns the given integer value in the "0x..." hexadecimal format.
//
// The actual number of digits depends on the number value.
//

std::string HexDumpUnknownSizeInteger(int64_t value);

std::string HexDumpUnknownSizeInteger(uint64_t value);

//
// Returns space-separated hex dumps of the specified memory bytes.
//

std::string HexDumpBytes(const void* begin, int64_t size);

std::string HexDumpBytes(const std::vector<uint8_t>& bytes);

}  // namespace google_smart_card

#endif  // GOOGLE_SMART_CARD_COMMON_LOGGING_HEX_DUMP_H_
