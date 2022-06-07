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

#include <google_smart_card_common/logging/hex_dumping.h>

namespace google_smart_card {

TEST(LoggingHexDumpingTest, HexDumpByte) {
  EXPECT_EQ("0x00", HexDumpByte(static_cast<int8_t>(0)));
  EXPECT_EQ("0x00", HexDumpByte(static_cast<uint8_t>(0)));

  EXPECT_EQ("0x01", HexDumpByte(static_cast<int8_t>(1)));
  EXPECT_EQ("0x01", HexDumpByte(static_cast<uint8_t>(1)));

  EXPECT_EQ("0x64", HexDumpByte(static_cast<int8_t>(100)));
  EXPECT_EQ("0x64", HexDumpByte(static_cast<uint8_t>(100)));

  EXPECT_EQ("0xFF", HexDumpByte(static_cast<int8_t>(-1)));
  EXPECT_EQ("0xFF", HexDumpByte(static_cast<uint8_t>(255)));
}

TEST(LoggingHexDumpingTest, HexDumpDoublet) {
  EXPECT_EQ("0x0000", HexDumpDoublet(static_cast<int16_t>(0)));
  EXPECT_EQ("0x0000", HexDumpDoublet(static_cast<uint16_t>(0)));

  EXPECT_EQ("0x0001", HexDumpDoublet(static_cast<int16_t>(1)));
  EXPECT_EQ("0x0001", HexDumpDoublet(static_cast<uint16_t>(1)));

  EXPECT_EQ("0xFFFF", HexDumpDoublet(static_cast<int16_t>(-1)));
  EXPECT_EQ("0xFFFF", HexDumpDoublet(static_cast<uint16_t>(-1)));
}

TEST(LoggingHexDumpingTest, HexDumpQuadlet) {
  EXPECT_EQ("0x00000000", HexDumpQuadlet(static_cast<int32_t>(0)));
  EXPECT_EQ("0x00000000", HexDumpQuadlet(static_cast<uint32_t>(0)));

  EXPECT_EQ("0x00000001", HexDumpQuadlet(static_cast<int32_t>(1)));
  EXPECT_EQ("0x00000001", HexDumpQuadlet(static_cast<uint32_t>(1)));

  EXPECT_EQ("0xFFFFFFFF", HexDumpQuadlet(static_cast<int32_t>(-1)));
  EXPECT_EQ("0xFFFFFFFF", HexDumpQuadlet(static_cast<uint32_t>(-1)));
}

TEST(LoggingHexDumpingTest, HexDumpOctlet) {
  EXPECT_EQ("0x0000000000000000", HexDumpOctlet(static_cast<int64_t>(0)));
  EXPECT_EQ("0x0000000000000000", HexDumpOctlet(static_cast<uint64_t>(0)));

  EXPECT_EQ("0x0000000000000001", HexDumpOctlet(static_cast<int64_t>(1)));
  EXPECT_EQ("0x0000000000000001", HexDumpOctlet(static_cast<uint64_t>(1)));

  EXPECT_EQ("0xFFFFFFFFFFFFFFFF", HexDumpOctlet(static_cast<int64_t>(-1)));
  EXPECT_EQ("0xFFFFFFFFFFFFFFFF", HexDumpOctlet(static_cast<uint64_t>(-1)));
}

TEST(LoggingHexDumpingTest, HexDumpInteger) {
  EXPECT_EQ("0x00", HexDumpInteger(static_cast<int8_t>(0)));
  EXPECT_EQ("0x0C", HexDumpInteger(static_cast<int8_t>(12)));
  EXPECT_EQ("0x00", HexDumpInteger(static_cast<uint8_t>(0)));
  EXPECT_EQ("0x22", HexDumpInteger(static_cast<uint8_t>(34)));

  EXPECT_EQ("0x00000000", HexDumpInteger(static_cast<int32_t>(0)));
  EXPECT_EQ("0x000004D2", HexDumpInteger(static_cast<int32_t>(1234)));
  EXPECT_EQ("0x00000000", HexDumpInteger(static_cast<uint32_t>(0)));
  EXPECT_EQ("0x000011D7", HexDumpInteger(static_cast<uint32_t>(4567)));

  EXPECT_EQ("0x0000000000000000", HexDumpInteger(static_cast<int64_t>(0)));
  EXPECT_EQ("0x000000000000007B", HexDumpInteger(static_cast<int64_t>(123)));
  EXPECT_EQ("0x0000000000000000", HexDumpInteger(static_cast<uint64_t>(0)));
  EXPECT_EQ("0x00000000000001C8", HexDumpInteger(static_cast<uint64_t>(456)));
}

TEST(LoggingHexDumpingTest, HexDumpUnknownSizeInteger) {
  EXPECT_EQ("0x00", HexDumpUnknownSizeInteger(static_cast<int64_t>(0)));
  EXPECT_EQ("0x00", HexDumpUnknownSizeInteger(static_cast<uint64_t>(0)));

  EXPECT_EQ("0xFF", HexDumpUnknownSizeInteger(static_cast<int64_t>(255)));
  EXPECT_EQ("0xFF", HexDumpUnknownSizeInteger(static_cast<uint64_t>(255)));

  EXPECT_EQ("0x00000100", HexDumpUnknownSizeInteger(static_cast<int64_t>(256)));
  EXPECT_EQ("0x00000100",
            HexDumpUnknownSizeInteger(static_cast<uint64_t>(256)));

  EXPECT_EQ("0xFFFFFFFF",
            HexDumpUnknownSizeInteger(static_cast<int64_t>((1LL << 32) - 1)));
  EXPECT_EQ(HexDumpUnknownSizeInteger(static_cast<uint64_t>((1LL << 32) - 1)),
            "0xFFFFFFFF");

  EXPECT_EQ("0x0000000100000000",
            HexDumpUnknownSizeInteger(static_cast<int64_t>(1LL << 32)));
  EXPECT_EQ("0x0000000100000000",
            HexDumpUnknownSizeInteger(static_cast<uint64_t>(1LL << 32)));

  EXPECT_EQ("0x7FFFFFFFFFFFFFFF",
            HexDumpUnknownSizeInteger(std::numeric_limits<int64_t>::max()));
  EXPECT_EQ("0xFFFFFFFFFFFFFFFF",
            HexDumpUnknownSizeInteger(static_cast<uint64_t>(-1)));

  EXPECT_EQ("0xFF", HexDumpUnknownSizeInteger(static_cast<int64_t>(-1)));
  EXPECT_EQ("0xFFFFFF00",
            HexDumpUnknownSizeInteger(static_cast<int64_t>(-256)));
  EXPECT_EQ("0x8000000000000000",
            HexDumpUnknownSizeInteger(std::numeric_limits<int64_t>::min()));
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
