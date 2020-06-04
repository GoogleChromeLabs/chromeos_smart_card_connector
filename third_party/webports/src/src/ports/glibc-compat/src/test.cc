/* Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/file.h>
#include <err.h>
#include <time.h>

#ifndef __GLIBC__
#include <sys/endian.h>
#endif

#include "gtest/gtest.h"

TEST(TestMktemp, mkdtemp_errors) {
  char small[] = "small";
  char missing_template[] = "missing_template";
  char short_template_XXX[] = "short_template_XXX";
  char not_XXXXXX_suffix[] = "not_XXXXXX_suffix";
  ASSERT_EQ((char*)NULL, mkdtemp(small));
  ASSERT_EQ(EINVAL, errno);
  ASSERT_EQ((char*)NULL, mkdtemp(missing_template));
  ASSERT_EQ(EINVAL, errno);
  ASSERT_EQ((char*)NULL, mkdtemp(short_template_XXX));
  ASSERT_EQ(EINVAL, errno);
  ASSERT_EQ((char*)NULL, mkdtemp(not_XXXXXX_suffix));
  ASSERT_EQ(EINVAL, errno);
}

TEST(TestMktemp, mkdtemp) {
  char tempdir[] = "tempfile_XXXXXX";
  ASSERT_NE((char*)NULL, mkdtemp(tempdir));
  // Check that tempname starts with the correct prefix but has been
  // modified from the original.
  ASSERT_EQ(0, strncmp("tempfile_", tempdir, strlen("tempfile_")));
  ASSERT_STRNE("tempfile_XXXXXX", tempdir);

  // Check the directory exists
  struct stat buf;
  ASSERT_EQ(0, stat(tempdir, &buf));
  ASSERT_TRUE(S_ISDIR(buf.st_mode));
  ASSERT_EQ(0, rmdir(tempdir));
}

TEST(TestMktemp, mkstemp) {
  char tempfile[] = "tempfile_XXXXXX";
  int fd = mkstemp(tempfile);
  ASSERT_GT(fd, -1);

  // Check that tempname starts with the correct prefix but has been
  // modified from the original.
  ASSERT_EQ(0, strncmp("tempfile_", tempfile, strlen("tempfile_")));
  ASSERT_STRNE("tempfile_XXXXXX", tempfile);

  ASSERT_EQ(4, write(fd, "test", 4));
  ASSERT_EQ(0, close(fd));
}

TEST(TestEndian, byte_order) {
#ifdef __native_client__
  ASSERT_EQ(LITTLE_ENDIAN, BYTE_ORDER);
  ASSERT_EQ(LITTLE_ENDIAN, BYTE_ORDER);
#endif
}

#ifndef __GLIBC__
TEST(TestEndian, byte_swap) {
  // Test BSD byte-swapping macros
  uint16_t num16 = 0x0102u;
  uint32_t num32 = 0x01020304u;
  ASSERT_EQ(num16, htole16(num16));
  ASSERT_EQ(num32, htole32(num32));
  ASSERT_EQ(num16, letoh16(num16));
  ASSERT_EQ(num32, letoh32(num32));

  // Can't inline these in the ASSERT_EQ statements as they can be
  // 'statement expressions' (on bionic at least) which can't be
  // template parameters apparently.
  int n16 = htons(num16);
  int n32 = htonl(num32);
  int h16 = ntohs(num16);
  int h32 = ntohl(num32);
  ASSERT_EQ(n16, htobe16(num16));
  ASSERT_EQ(n32, htobe32(num32));
  ASSERT_EQ(h16, betoh16(num16));
  ASSERT_EQ(h32, betoh32(num32));
}
#endif

// readv is not implemented in glibc.
#ifndef __GLIBC__
TEST(TestReadv, readv_writev) {
  struct iovec write_iov[3];
  struct iovec read_iov[3];
  char first[28], second[28], third[12];
  read_iov[0].iov_base = first;
  read_iov[0].iov_len = sizeof(first);
  read_iov[1].iov_base = second;
  read_iov[1].iov_len = sizeof(second);
  read_iov[2].iov_base = third;
  read_iov[2].iov_len = sizeof(third);
  int i = 0;
  ssize_t ret = 0;
  const char* str1 = "abcdefghijklmnopqrstuvwxyz\n";
  const char* str2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
  const char* str3 = "0123456789\n";
  write_iov[0].iov_base = (void*)str1;
  write_iov[0].iov_len = strlen(str1) + 1;
  write_iov[1].iov_base = (void*)str2;
  write_iov[1].iov_len = strlen(str2) + 1;
  write_iov[2].iov_base = (void*)str3;
  write_iov[2].iov_len = strlen(str3) + 1;
  int fd = open("test.txt", O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR);
  ASSERT_NE(fd, -1);
  ret = writev(fd, write_iov, 3);
  ASSERT_NE(ret, -1);
  ASSERT_NE(close(fd), -1);
  fd = open("test.txt", O_RDONLY);
  ASSERT_NE(fd, -1);
  ret = readv(fd, read_iov, 3);
  ASSERT_NE(ret, -1);
  ASSERT_STREQ(str1, (const char*)read_iov[0].iov_base);
  ASSERT_EQ(28, read_iov[0].iov_len);
  ASSERT_STREQ(str2, (const char*)read_iov[1].iov_base);
  ASSERT_EQ(28, read_iov[1].iov_len);
  ASSERT_STREQ(str3, (const char*)read_iov[2].iov_base);
  ASSERT_EQ(12, read_iov[2].iov_len);
  ASSERT_NE(close(fd), -1);
  ASSERT_NE(-1, unlink("test.txt"));
}
#endif

// No tests for funtions ended with at, e.g. openat, fts_*
// fchdir is not implemented in sel_ldr
#if 0
TEST(TestOpenat, openat) {
  ASSERT_NE(-1, mkdir("t1", S_IRWXU | S_IRWXG | S_IXGRP));
  DIR* dir = opendir("t1");
  ASSERT_NE((DIR*)NULL, dir);
  int fd_t1 = dirfd(dir);
  ASSERT_NE(-1, fd_t1);
  int fd_t2 = openat(fd_t1, "test.txt", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
  ASSERT_NE(-1, fd_t2);
  ASSERT_EQ(4, write(fd_t2, "test", 4));
  ASSERT_NE(-1, close(fd_t2));
  ASSERT_NE(-1, close(fd_t1));
  ASSERT_NE(-1, unlink("t1/test.txt"));
  ASSERT_NE(-1, rmdir("t1"));
}
#endif

TEST(TestLockf, lockf) {
  // The fcntl() method underlying lockf() is not implemented in NaCl.
  ASSERT_EQ(-1, lockf(1, F_LOCK, 1));
  ASSERT_EQ(ENOSYS, errno);
  ASSERT_EQ(-1, lockf(1, F_TLOCK, 1));
  ASSERT_EQ(ENOSYS, errno);
  ASSERT_EQ(-1, lockf(1, F_ULOCK, 1));
  ASSERT_EQ(ENOSYS, errno);
}

TEST(TestFlock, flock) {
  // The fcntl() method underlying flock() is not implemented in NaCl.
  ASSERT_EQ(-1, flock(1, LOCK_SH));
  ASSERT_EQ(ENOSYS, errno);
  ASSERT_EQ(-1, flock(1, LOCK_EX));
  ASSERT_EQ(ENOSYS, errno);
  ASSERT_EQ(-1, flock(1, LOCK_UN));
  ASSERT_EQ(ENOSYS, errno);
}

/* Calling err will cause the test program exit with failure code if errno
 * is set. No tests for err.
 */

TEST(TestTimegm, timegm) {
  struct tm tmp;
  tmp.tm_sec = 1;
  tmp.tm_min = 0;
  tmp.tm_hour = 0;
  tmp.tm_mday = 3;
  tmp.tm_mon = 4 - 1;
  tmp.tm_year = 2015 - 1900;
  tmp.tm_isdst = -1;
  time_t tt = timegm(&tmp);
  ASSERT_EQ(1428019201, tt);
}

int main(int argc, char** argv) {
  setenv("TERM", "xterm-256color", 0);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
