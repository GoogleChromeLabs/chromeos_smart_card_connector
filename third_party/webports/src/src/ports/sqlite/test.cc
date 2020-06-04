// Copyright 2015 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <stdio.h>
#include <sys/mount.h>
#include <unistd.h>

// the libary we are testing.
#include <sqlite3.h>

#include "nacl_io/nacl_io.h"

#include "gtest/gtest.h"

TEST(SqliteTest, InsertValues) {
  nacl_io_init();
  umount("/");
  ASSERT_EQ(mount( "", "/", "memfs", 0, NULL), 0);

  sqlite3 *db = NULL;
  ASSERT_EQ(sqlite3_open("/test.db", &db), SQLITE_OK);
  ASSERT_EQ(sqlite3_exec(db, "CREATE TABLE foo(bar INTEGER, baz INTEGER)", NULL,
                         NULL, NULL), SQLITE_OK);
  EXPECT_EQ(sqlite3_exec(db, "INSERT INTO foo(bar, baz) VALUES (1, 2)",
                         NULL, NULL, NULL), SQLITE_OK);
  EXPECT_EQ(sqlite3_exec(db, "INSERT INTO foo(bar, baz) VALUES (3, 4)",
                         NULL, NULL, NULL), SQLITE_OK);
  ASSERT_EQ(sqlite3_close(db), SQLITE_OK);
}

int main(int argc, char** argv) {
  setenv("TERM", "xterm-256color", 0);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
