// Copyright 2021 Google Inc. All Rights Reserved.
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

#include <google_smart_card_common/ipc_emulation.h>

#include <stdint.h>

#include <gtest/gtest.h>

namespace google_smart_card {

class IpcEmulationTest : public testing::Test {
 protected:
  IpcEmulationTest() { IpcEmulation::CreateGlobalInstance(); }
  ~IpcEmulationTest() override {
    IpcEmulation::DestroyGlobalInstance();
  }

  IpcEmulation* ipc_emulation() { return IpcEmulation::GetInstance(); }
};

TEST_F(IpcEmulationTest, CreateAndClose) {
  int fd1 = -1, fd2 = -1;
  ipc_emulation()->CreateInMemoryFilePair(&fd1, &fd2,
                                          /*reads_should_block=*/false);
  EXPECT_NE(fd1, -1);
  EXPECT_NE(fd2, -1);
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd1));
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd2));
}

TEST_F(IpcEmulationTest, CloseUnknownFile) {
  EXPECT_FALSE(ipc_emulation()->CloseInMemoryFile(123));
}

TEST_F(IpcEmulationTest, DoubleClose) {
  int fd1 = -1, fd2 = -1;
  ipc_emulation()->CreateInMemoryFilePair(&fd1, &fd2,
                                          /*reads_should_block=*/false);
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd1));
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd2));
  EXPECT_FALSE(ipc_emulation()->CloseInMemoryFile(fd1));
  EXPECT_FALSE(ipc_emulation()->CloseInMemoryFile(fd2));
}

TEST_F(IpcEmulationTest, WriteAndNonBlockingRead) {
  const int kDataSize = 3;
  const uint8_t kDataToWrite[kDataSize] = {1, 3, 255};
  const int kReadBufferSize = 10;

  int fd1 = -1, fd2 = -1;
  ipc_emulation()->CreateInMemoryFilePair(&fd1, &fd2,
                                          /*reads_should_block=*/false);

  // Reading returns no data when nothing was written.
  uint8_t read_buffer[kReadBufferSize];
  int64_t read_count = kReadBufferSize;
  EXPECT_EQ(
      ipc_emulation()->ReadFromInMemoryFile(fd2, read_buffer, &read_count),
      IpcEmulation::ReadResult::kNoData);

  // Data is written.
  EXPECT_TRUE(
      ipc_emulation()->WriteToInMemoryFile(fd1, kDataToWrite, kDataSize));

  // The written data is read back.
  read_count = kReadBufferSize;
  EXPECT_EQ(
      ipc_emulation()->ReadFromInMemoryFile(fd2, read_buffer, &read_count),
      IpcEmulation::ReadResult::kSuccess);
  EXPECT_EQ(read_count, kDataSize);
  EXPECT_EQ(memcmp(read_buffer, kDataToWrite, kDataSize), 0);

  // Reading returns no data when all previously written data has been read.
  read_count = kReadBufferSize;
  EXPECT_EQ(
      ipc_emulation()->ReadFromInMemoryFile(fd2, read_buffer, &read_count),
      IpcEmulation::ReadResult::kNoData);

  // The files are closed.
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd1));
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd2));

  // Reading from and writing to closed files fails.
  read_count = kReadBufferSize;
  EXPECT_EQ(
      ipc_emulation()->ReadFromInMemoryFile(fd2, read_buffer, &read_count),
      IpcEmulation::ReadResult::kNoSuchFile);
  EXPECT_FALSE(
      ipc_emulation()->WriteToInMemoryFile(fd1, kDataToWrite, kDataSize));
}

TEST_F(IpcEmulationTest, WriteAndBlockingRead) {
  const int kDataSize = 3;
  const uint8_t kDataToWrite[kDataSize] = {1, 3, 255};
  const int kReadBufferSize = 10;

  int fd1 = -1, fd2 = -1;
  ipc_emulation()->CreateInMemoryFilePair(&fd1, &fd2,
                                          /*reads_should_block=*/true);

  // Data is written.
  EXPECT_TRUE(
      ipc_emulation()->WriteToInMemoryFile(fd1, kDataToWrite, kDataSize));

  // The written data is read back.
  uint8_t read_buffer[kReadBufferSize];
  int64_t read_count = kReadBufferSize;
  EXPECT_EQ(
      ipc_emulation()->ReadFromInMemoryFile(fd2, read_buffer, &read_count),
      IpcEmulation::ReadResult::kSuccess);
  EXPECT_EQ(read_count, kDataSize);
  EXPECT_EQ(memcmp(read_buffer, kDataToWrite, kDataSize), 0);

  // The files are closed.
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd1));
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd2));

  // Reading from closed files fails.
  read_count = kReadBufferSize;
  EXPECT_EQ(
      ipc_emulation()->ReadFromInMemoryFile(fd2, read_buffer, &read_count),
      IpcEmulation::ReadResult::kNoSuchFile);
}

TEST_F(IpcEmulationTest, WriteAndReadZeroBytes) {
  int fd1 = -1, fd2 = -1;
  ipc_emulation()->CreateInMemoryFilePair(&fd1, &fd2,
                                          /*reads_should_block=*/false);

  EXPECT_TRUE(ipc_emulation()->WriteToInMemoryFile(fd1, nullptr, 0));
  int64_t read_count = 0;
  EXPECT_EQ(ipc_emulation()->ReadFromInMemoryFile(fd2, nullptr, &read_count),
            IpcEmulation::ReadResult::kSuccess);

  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd1));
  EXPECT_TRUE(ipc_emulation()->CloseInMemoryFile(fd2));
}

}  // namespace google_smart_card
