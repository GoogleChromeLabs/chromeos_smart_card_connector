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

#include <google_smart_card_common/logging/hex_dumping.h>

#include <limits>
#include <sstream>

#include <google_smart_card_common/logging/logging.h>
#include <google_smart_card_common/numeric_conversions.h>

namespace google_smart_card {

namespace {

constexpr int kBitsPerHexDigit = 4;

template <typename T>
std::string HexDumpIntegerWithExactBitLength(T value, int bit_length) {
  // We want to dump negative numbers in their two-complement representation,
  // made with the specified bit_length. E.g. when value=-1 and bit_length=8,
  // we want to dump the "FF" characters (but not "-1" or "FFFFFFFFFFFFFFFF").
  //
  // In order to accomplish that, at first the number is casted into an unsigned
  // 64-bit number (which effectively means 64-bit two-complement
  // representation), and then the adjustment of the negative numbers is made
  // when necessary (if the original bit length was smaller than 64).
  uint64_t value_to_dump = static_cast<uint64_t>(value);
  if (value < 0 && bit_length < 64) value_to_dump += 1ULL << bit_length;

  std::ostringstream stream;
  stream.setf(std::ios::uppercase);
  stream.setf(std::ios::hex, std::ios::basefield);
  stream.width(bit_length / kBitsPerHexDigit);
  stream.fill('0');
  stream << value_to_dump;
  return "0x" + stream.str();
}

template <typename T>
int GuessIntegerBitLength(T value) {
  if (CompareIntegers(std::numeric_limits<int8_t>::min(), value) <= 0 &&
      CompareIntegers(std::numeric_limits<uint8_t>::max(), value) >= 0) {
    return 8;
  }
  if (CompareIntegers(std::numeric_limits<int32_t>::min(), value) <= 0 &&
      CompareIntegers(std::numeric_limits<uint32_t>::max(), value) >= 0) {
    return 32;
  }
  return 64;
}

}  // namespace

std::string HexDumpByte(int8_t value) {
  return HexDumpIntegerWithExactBitLength(value, 8);
}

std::string HexDumpByte(uint8_t value) {
  return HexDumpIntegerWithExactBitLength(value, 8);
}

std::string HexDumpDoublet(int16_t value) {
  return HexDumpIntegerWithExactBitLength(value, 16);
}

std::string HexDumpDoublet(uint16_t value) {
  return HexDumpIntegerWithExactBitLength(value, 16);
}

std::string HexDumpQuadlet(int32_t value) {
  return HexDumpIntegerWithExactBitLength(value, 32);
}

std::string HexDumpQuadlet(uint32_t value) {
  return HexDumpIntegerWithExactBitLength(value, 32);
}

std::string HexDumpOctlet(int64_t value) {
  return HexDumpIntegerWithExactBitLength(value, 64);
}

std::string HexDumpOctlet(uint64_t value) {
  return HexDumpIntegerWithExactBitLength(value, 64);
}

std::string HexDumpPointer(const void* value) {
  if (!value) return "NULL";
  return HexDumpInteger(reinterpret_cast<uintptr_t>(value));
}

std::string HexDumpUnknownSizeInteger(int64_t value) {
  return HexDumpIntegerWithExactBitLength(value, GuessIntegerBitLength(value));
}

std::string HexDumpUnknownSizeInteger(uint64_t value) {
  return HexDumpIntegerWithExactBitLength(value, GuessIntegerBitLength(value));
}

std::string HexDumpBytes(const void* begin, int64_t size) {
  if (size) GOOGLE_SMART_CARD_CHECK(begin);
  const uint8_t* const begin_casted = static_cast<const uint8_t*>(begin);
  std::string result;
  for (int64_t index = 0; index < size; ++index) {
    if (index) result += ' ';
    result += HexDumpByte(begin_casted[index]);
  }
  return result;
}

std::string HexDumpBytes(const std::vector<uint8_t>& bytes) {
  if (bytes.empty()) return "";
  return HexDumpBytes(&bytes[0], bytes.size());
}

}  // namespace google_smart_card
