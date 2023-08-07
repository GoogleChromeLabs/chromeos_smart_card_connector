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

#include <stdint.h>

#include <string>

#include <gtest/gtest.h>

#include "common/cpp/src/google_smart_card_common/logging/hex_dumping.h"

namespace google_smart_card {

TEST(LoggingHexDumpingTest, HexDumpByte) {
  EXPECT_EQ(HexDumpByte(static_cast<int8_t>(0)), "0x00");
  EXPECT_EQ(HexDumpByte(static_cast<uint8_t>(0)), "0x00");

  EXPECT_EQ(HexDumpByte(static_cast<int8_t>(1)), "0x01");
  EXPECT_EQ(HexDumpByte(static_cast<uint8_t>(1)), "0x01");

  EXPECT_EQ(HexDumpByte(static_cast<int8_t>(100)), "0x64");
  EXPECT_EQ(HexDumpByte(static_cast<uint8_t>(100)), "0x64");

  EXPECT_EQ(HexDumpByte(static_cast<int8_t>(-1)), "0xFF");
  EXPECT_EQ(HexDumpByte(static_cast<uint8_t>(255)), "0xFF");
}

TEST(LoggingHexDumpingTest, HexDumpDoublet) {
  EXPECT_EQ(HexDumpDoublet(static_cast<int16_t>(0)), "0x0000");
  EXPECT_EQ(HexDumpDoublet(static_cast<uint16_t>(0)), "0x0000");

  EXPECT_EQ(HexDumpDoublet(static_cast<int16_t>(1)), "0x0001");
  EXPECT_EQ(HexDumpDoublet(static_cast<uint16_t>(1)), "0x0001");

  EXPECT_EQ(HexDumpDoublet(static_cast<int16_t>(-1)), "0xFFFF");
  EXPECT_EQ(HexDumpDoublet(static_cast<uint16_t>(-1)), "0xFFFF");
}

TEST(LoggingHexDumpingTest, HexDumpQuadlet) {
  EXPECT_EQ(HexDumpQuadlet(static_cast<int32_t>(0)), "0x00000000");
  EXPECT_EQ(HexDumpQuadlet(static_cast<uint32_t>(0)), "0x00000000");

  EXPECT_EQ(HexDumpQuadlet(static_cast<int32_t>(1)), "0x00000001");
  EXPECT_EQ(HexDumpQuadlet(static_cast<uint32_t>(1)), "0x00000001");

  EXPECT_EQ(HexDumpQuadlet(static_cast<int32_t>(-1)), "0xFFFFFFFF");
  EXPECT_EQ(HexDumpQuadlet(static_cast<uint32_t>(-1)), "0xFFFFFFFF");
}

TEST(LoggingHexDumpingTest, HexDumpOctlet) {
  EXPECT_EQ(HexDumpOctlet(static_cast<int64_t>(0)), "0x0000000000000000");
  EXPECT_EQ(HexDumpOctlet(static_cast<uint64_t>(0)), "0x0000000000000000");

  EXPECT_EQ(HexDumpOctlet(static_cast<int64_t>(1)), "0x0000000000000001");
  EXPECT_EQ(HexDumpOctlet(static_cast<uint64_t>(1)), "0x0000000000000001");

  EXPECT_EQ(HexDumpOctlet(static_cast<int64_t>(-1)), "0xFFFFFFFFFFFFFFFF");
  EXPECT_EQ(HexDumpOctlet(static_cast<uint64_t>(-1)), "0xFFFFFFFFFFFFFFFF");
}

TEST(LoggingHexDumpingTest, HexDumpInteger) {
  EXPECT_EQ(HexDumpInteger(static_cast<int8_t>(0)), "0x00");
  EXPECT_EQ(HexDumpInteger(static_cast<int8_t>(12)), "0x0C");
  EXPECT_EQ(HexDumpInteger(static_cast<uint8_t>(0)), "0x00");
  EXPECT_EQ(HexDumpInteger(static_cast<uint8_t>(34)), "0x22");

  EXPECT_EQ(HexDumpInteger(static_cast<int32_t>(0)), "0x00000000");
  EXPECT_EQ(HexDumpInteger(static_cast<int32_t>(1234)), "0x000004D2");
  EXPECT_EQ(HexDumpInteger(static_cast<uint32_t>(0)), "0x00000000");
  EXPECT_EQ(HexDumpInteger(static_cast<uint32_t>(4567)), "0x000011D7");

  EXPECT_EQ(HexDumpInteger(static_cast<int64_t>(0)), "0x0000000000000000");
  EXPECT_EQ(HexDumpInteger(static_cast<int64_t>(123)), "0x000000000000007B");
  EXPECT_EQ(HexDumpInteger(static_cast<uint64_t>(0)), "0x0000000000000000");
  EXPECT_EQ(HexDumpInteger(static_cast<uint64_t>(456)), "0x00000000000001C8");
}

TEST(LoggingHexDumpingTest, HexDumpUnknownSizeInteger) {
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<int64_t>(0)), "0x00");
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<uint64_t>(0)), "0x00");

  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<int64_t>(255)), "0xFF");
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<uint64_t>(255)), "0xFF");

  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<int64_t>(256)), "0x00000100");
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<uint64_t>(256)),
            "0x00000100");

  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<int64_t>((1LL << 32) - 1)),
            "0xFFFFFFFF");
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<uint64_t>((1LL << 32) - 1)),
            "0xFFFFFFFF");

  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<int64_t>(1LL << 32)),
            "0x0000000100000000");
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<uint64_t>(1LL << 32)),
            "0x0000000100000000");

  EXPECT_EQ(HexDumpUnknownSizeInteger(std::numeric_limits<int64_t>::max()),
            "0x7FFFFFFFFFFFFFFF");
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<uint64_t>(-1)),
            "0xFFFFFFFFFFFFFFFF");

  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<int64_t>(-1)), "0xFF");
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<int64_t>(-256)),
            "0xFFFFFF00");
  EXPECT_EQ(HexDumpUnknownSizeInteger(std::numeric_limits<int64_t>::min()),
            "0x8000000000000000");
}

TEST(LoggingHexDumpingTest, HexDumpBytes) {
  EXPECT_EQ(HexDumpBytes(nullptr, 0), "");

  const uint8_t kArray[] = {1, 2, 123};
  EXPECT_EQ(HexDumpBytes(kArray, 3), "0x01 0x02 0x7B");
}

TEST(LoggingHexDumpingTest, HexDumpBytesVector) {
  EXPECT_EQ(HexDumpBytes({}), "");
  EXPECT_EQ(HexDumpBytes({1, 2, 123}), "0x01 0x02 0x7B");
}

}  // namespace google_smart_card
