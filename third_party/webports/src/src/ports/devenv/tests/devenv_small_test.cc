// Copyright (c) 2014 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"

#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <unistd.h>

static char *argv0;

// Make sure that the plumbing works.
TEST(Plumbing, Identity) {
  EXPECT_EQ(0, 0);
}

// Test process functions.
TEST(Plumbing, ProcessTests) {
  int pid = getpid();
  EXPECT_GT(pid, 1);
  EXPECT_GT(getppid(), 0);
  EXPECT_EQ(setpgrp(), 0);
  EXPECT_EQ(getpgid(0), pid);
  EXPECT_EQ(getpgrp(), pid);
  EXPECT_EQ(setsid(), -1);
}

// Used in main to allow the test exectuable to be started
// as a subprocess.
// Child takes args:
// ./test return <return-code> <expected-foo-env>
// It will return 55 as a bad return value in the case
// that the FOO environment variable isn't the expected
// value.
static int return_child(int argc, char **argv) {
  if (strcmp(argv[3], getenv("FOO")) != 0) {
    return 55;
  }
  return atoi(argv[2]);
}

#define ARGV_FOR_CHILD(x) \
  char *argv[5]; \
  argv[0] = argv0; \
  argv[1] = const_cast<char*>("return"); \
  argv[2] = const_cast<char*>("111"); \
  argv[3] = const_cast<char*>(x); \
  argv[4] = NULL;

#define ENVP_FOR_CHILD(x) \
  char *envp[2]; \
  envp[0] = const_cast<char*>(x); \
  envp[1] = NULL;

TEST(Spawn, spawnve) {
  int status;
  ARGV_FOR_CHILD("spawnve");
  ENVP_FOR_CHILD("FOO=spawnve");
  pid_t pid = spawnve(P_NOWAIT, argv0, argv, envp);
  ASSERT_GE(pid, 0);
  pid_t npid = waitpid(pid, &status, 0);
  EXPECT_EQ(pid, npid);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(111, WEXITSTATUS(status));
}

#define VFORK_SETUP_SPAWN \
  int status; \
  pid_t pid = vfork(); \
  ASSERT_GE(pid, 0); \
  if (pid) { \
    pid_t npid = waitpid(pid, &status, 0); \
    EXPECT_EQ(pid, npid); \
    EXPECT_TRUE(WIFEXITED(status)); \
    EXPECT_EQ(111, WEXITSTATUS(status)); \
  } else

TEST(Vfork, execve) {
  VFORK_SETUP_SPAWN {
    ARGV_FOR_CHILD("execve");
    ENVP_FOR_CHILD("FOO=execve");
    execve(argv0, argv, envp);
  }
}

TEST(Vfork, execv) {
  setenv("FOO", "execv", 1);
  VFORK_SETUP_SPAWN {
    ARGV_FOR_CHILD("execv");
    execv(argv0, argv);
  }
}

TEST(Vfork, execvp) {
  setenv("FOO", "execvp", 1);
  VFORK_SETUP_SPAWN {
    ARGV_FOR_CHILD("execvp");
    execvp(argv0, argv);
  }
}

TEST(Vfork, execvpe) {
  VFORK_SETUP_SPAWN {
    ARGV_FOR_CHILD("execvpe");
    ENVP_FOR_CHILD("FOO=execvpe");
    execvpe(argv0, argv, envp);
  }
}

TEST(Vfork, execl) {
  setenv("FOO", "execl", 1);
  VFORK_SETUP_SPAWN {
    execl(argv0, argv0, "return", "111", "execl", NULL);
  }
}

TEST(Vfork, execlp) {
  setenv("FOO", "execlp", 1);
  VFORK_SETUP_SPAWN {
    execlp(argv0, argv0, "return", "111", "execlp", NULL);
  }
}

TEST(Vfork, execle) {
  VFORK_SETUP_SPAWN {
    ENVP_FOR_CHILD("FOO=execle");
    execle(argv0, argv0, "return", "111", "execle", NULL, envp);
  }
}

TEST(Vfork, execlpe) {
  VFORK_SETUP_SPAWN {
    ENVP_FOR_CHILD("FOO=execlpe");
    execlpe(argv0, argv0, "return", "111", "execlpe", NULL, envp);
  }
}

// Used in main to allow the test exectuable to be started
// as a subprocess.
// Child takes args:
// ./test _exit <return-code> <expected-foo-env>
// It will return 55 as a bad return value in the case
// that the FOO environment variable isn't the expected
// value.
static int exit_child(int argc, char **argv) {
  if (strcmp(argv[3], getenv("FOO")) != 0) {
    return 55;
  }
  _exit(atoi(argv[2]));
  // Shouldn't get here.
  return 55;
}

TEST(Vfork, exit) {
  int status;
  pid_t pid = vfork();
  ASSERT_GE(pid, 0);
  if (pid) {
    pid_t npid = waitpid(pid, &status, 0);
    EXPECT_EQ(pid, npid);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(123, WEXITSTATUS(status));
  } else {
    _exit(123);
  }
}

TEST(Vfork, RegularExit) {
  VFORK_SETUP_SPAWN {
    ENVP_FOR_CHILD("FOO=RegularExit");
    execlpe(argv0, argv0, "_exit", "111", "RegularExit", NULL, envp);
  }
}

// Used in main to allow the test exectuable to be started
// as an echo server.
// Child takes args:
// ./test pipes
static int pipes_child(int argc, char **argv) {
  char buffer[200];
  for (;;) {
    int len = read(0, buffer, sizeof(buffer));
    if (len <= 0) {
      break;
    }
    write(1, buffer, len);
  }
  close(1);
  close(0);
  return 0;
}

// Write to an echo process, get reply then close pipes.
TEST(Pipes, Echo) {
  int pipe_a[2];
  int pipe_b[2];
  // Create two pipe pairs pipe_a[1] ->  pipe_a[0]
  //                       pipe_b[1] ->  pipe_b[0]
  ASSERT_EQ(0, pipe(pipe_a));
  ASSERT_EQ(0, pipe(pipe_b));
  pid_t pid = vfork();
  ASSERT_GE(pid, 0);
  if (!pid) {
    // Dup two ends of the pipes into stdin + stdout of the echo process.
    ASSERT_EQ(0, dup2(pipe_a[0], 0));
    EXPECT_EQ(0, close(pipe_a[0]));
    EXPECT_EQ(0, close(pipe_a[1]));
    EXPECT_EQ(1, dup2(pipe_b[1], 1));
    EXPECT_EQ(0, close(pipe_b[0]));
    EXPECT_EQ(0, close(pipe_b[1]));
    execlp(argv0, argv0, "pipes", NULL);
    // Don't get here.
    ASSERT_TRUE(false);
  }

  EXPECT_EQ(0, close(pipe_a[0]));
  EXPECT_EQ(0, close(pipe_b[1]));

  const char test_message[] = "test message";

  // Write to pipe_a.
  int len = write(pipe_a[1], test_message, strlen(test_message));
  EXPECT_EQ(strlen(test_message), len);
  char buffer[100];
  // Wait for an echo back on pipe_b.
  int total = 0;
  while (total < strlen(test_message)) {
    len = read(pipe_b[0], buffer + total, sizeof(buffer));
    ASSERT_GE(len, 0);
    if (len == 0) break;
    total += len;
  }
  EXPECT_EQ(strlen(test_message), len);
  EXPECT_TRUE(memcmp(buffer, test_message, len) == 0);

  EXPECT_EQ(0, close(pipe_a[1]));
  EXPECT_EQ(0, close(pipe_b[0]));
}

// Try stdout with pipes.
TEST(Pipes, StdoutEcho) {
  int pipes[2];

  // Create pipe pair pipes[1] -> pipe[0]
  ASSERT_EQ(0, pipe(pipes));

  // Do a vfork, and do an echo from child.
  pid_t pid = vfork();
  ASSERT_GE(pid, 0);
  if (!pid) {
    // Tie child's stdout with pipe's write end.
    ASSERT_EQ(1, dup2(pipes[1], 1));
    EXPECT_EQ(0, close(pipes[0]));
    execlp(argv0, argv0, "echo", NULL);
  }

  EXPECT_EQ(0, close(pipes[1]));
  char check_msg[] = "test";
  char buffer[100];
  int total = 0, len;
  while (total < strlen(check_msg)) {
    len = read(pipes[0], buffer+total, sizeof(buffer));
    ASSERT_GE(len, 0);
    if (len == 0) break;
    total += len;
  }
  ASSERT_EQ(0, memcmp(buffer, check_msg, len));
  EXPECT_EQ(0, close(pipes[0]));
}

// Write to an echo process, close immediately, then wait for reply.
TEST(Pipes, PipeFastClose) {
  int pipe_a[2];
  int pipe_b[2];
  // Create two pipe pairs pipe_a[1] ->  pipe_a[0]
  //                       pipe_b[1] ->  pipe_b[0]
  ASSERT_EQ(0, pipe(pipe_a));
  ASSERT_EQ(0, pipe(pipe_b));
  pid_t pid = vfork();
  ASSERT_GE(pid, 0);
  if (!pid) {
    // Dup two ends of the pipes into stdin + stdout of the echo process.
    ASSERT_EQ(0, dup2(pipe_a[0], 0));
    EXPECT_EQ(0, close(pipe_a[0]));
    EXPECT_EQ(0, close(pipe_a[1]));
    EXPECT_EQ(1, dup2(pipe_b[1], 1));
    EXPECT_EQ(0, close(pipe_b[0]));
    EXPECT_EQ(0, close(pipe_b[1]));
    execlp(argv0, argv0, "pipes", NULL);
    // Don't get here.
    ASSERT_TRUE(false);
  }

  EXPECT_EQ(0, close(pipe_a[0]));
  EXPECT_EQ(0, close(pipe_b[1]));

  const char test_message[] = "test message";

  // Write to pipe_a.
  int len = write(pipe_a[1], test_message, strlen(test_message));
  EXPECT_EQ(strlen(test_message), len);
  EXPECT_EQ(0, close(pipe_a[1]));

  char buffer[100];
  // Wait for an echo back on pipe_b.
  int total = 0;
  while (total < strlen(test_message)) {
    len = read(pipe_b[0], buffer + total, sizeof(buffer));
    ASSERT_GE(len, 0);
    if (len == 0) break;
    total += len;
  }
  EXPECT_EQ(strlen(test_message), len);
  EXPECT_TRUE(memcmp(buffer, test_message, len) == 0);

  EXPECT_EQ(0, close(pipe_b[0]));
}

// Write to a chain of echo process, get reply then close pipes.
TEST(Pipes, EchoChain) {
  int pipe_a[2];
  ASSERT_EQ(0, pipe(pipe_a));
  int pipe_b[2];
  ASSERT_EQ(0, pipe(pipe_b));

  pid_t pid = vfork();
  ASSERT_GE(pid, 0);
  if (!pid) {
    // Dup two ends of the pipes into stdin + stdout of the echo process.
    ASSERT_EQ(0, dup2(pipe_a[0], 0));
    EXPECT_EQ(0, close(pipe_a[0]));
    EXPECT_EQ(0, close(pipe_a[1]));
    EXPECT_EQ(1, dup2(pipe_b[1], 1));
    EXPECT_EQ(0, close(pipe_b[0]));
    EXPECT_EQ(0, close(pipe_b[1]));
    execlp(argv0, argv0, "pipes", NULL);
    // Don't get here.
    ASSERT_TRUE(false);
  }
  EXPECT_EQ(0, close(pipe_a[0]));
  EXPECT_EQ(0, close(pipe_b[1]));

  int pipe_c[2];
  ASSERT_EQ(0, pipe(pipe_c));

  pid_t pid2 = vfork();
  ASSERT_GE(pid2, 0);
  if (!pid2) {
    // Dup two ends of the pipes into stdin + stdout of the echo process.
    EXPECT_EQ(0, close(pipe_a[1]));
    ASSERT_EQ(0, dup2(pipe_b[0], 0));
    EXPECT_EQ(0, close(pipe_b[0]));
    EXPECT_EQ(1, dup2(pipe_c[1], 1));
    EXPECT_EQ(0, close(pipe_c[0]));
    EXPECT_EQ(0, close(pipe_c[1]));
    execlp(argv0, argv0, "pipes", NULL);
    // Don't get here.
    ASSERT_TRUE(false);
  }
  EXPECT_EQ(0, close(pipe_b[0]));
  EXPECT_EQ(0, close(pipe_c[1]));

  const char test_message[] = "test message";

  // Write to pipe_a.
  int len = write(pipe_a[1], test_message, strlen(test_message));
  EXPECT_EQ(strlen(test_message), len);
  char buffer[100];
  // Wait for an echo back on pipe_c.
  int total = 0;
  while (total < strlen(test_message)) {
    len = read(pipe_c[0], buffer + total, sizeof(buffer));
    ASSERT_GE(len, 0);
    if (len == 0) break;
    total += len;
  }
  EXPECT_EQ(strlen(test_message), len);
  EXPECT_TRUE(memcmp(buffer, test_message, len) == 0);

  EXPECT_EQ(0, close(pipe_a[1]));
  EXPECT_EQ(0, close(pipe_c[0]));
}

TEST(Pipes, NullFeof) {
  int p[2];
  ASSERT_EQ(0, pipe(p));
  ASSERT_EQ(0, close(p[1]));
  ASSERT_EQ(0, dup2(p[0], 0));
  while (!feof(stdin)) {
    int ch = fgetc(stdin);
  }
}

TEST(Pipes, Null) {
  int p[2];
  ASSERT_EQ(0, pipe(p));
  ASSERT_EQ(0, close(p[1]));
  char buffer[100];
  ssize_t len = read(p[0], buffer, sizeof(buffer));
  EXPECT_EQ(0, len);
  ASSERT_EQ(0, close(p[0]));
}

static int cloexec_check_child(int argc, char *argv[]) {
  int fd1;
  int fd2;
  struct stat st;

  if (sscanf(argv[2], "%d", &fd1) != 1)
    return 1;
  if (sscanf(argv[3], "%d", &fd2) != 1)
    return 1;
  if (fstat(fd1, &st) != 0)
    return 1;
  if (fcntl(fd1, F_GETFD) & FD_CLOEXEC)
    return 1;
  if (fstat(fd2, &st) != -1)
    return 1;
  if (errno != EBADF)
    return 1;
  return 42;
}

// Confirm FD_CLOEXEC works for pipes.
TEST(Pipes, CloseExec) {
  int p[2];
  ASSERT_EQ(0, pipe(p));

  char fd1[20];
  sprintf(fd1, "%d", p[0]);
  char fd2[20];
  sprintf(fd2, "%d", p[1]);

  fcntl(p[1], F_SETFD, fcntl(p[1], F_GETFD) | FD_CLOEXEC);

  pid_t pid = vfork();
  ASSERT_GE(pid, 0);
  if (!pid) {
    // When running in the child.
    execlp(argv0, argv0, "cloexec_check", fd1, fd2, NULL);
    // Don't get here.
    ASSERT_TRUE(false);
  }

  EXPECT_EQ(0, close(p[0]));
  EXPECT_EQ(0, close(p[1]));

  int status;
  pid_t npid = waitpid(pid, &status, 0);
  EXPECT_EQ(pid, npid);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(42, WEXITSTATUS(status));
}

extern "C" int nacl_main(int argc, char **argv) {
  if (argc == 4 && strcmp(argv[1], "return") == 0) {
    return return_child(argc, argv);
  } else if (argc == 4 && strcmp(argv[1], "_exit") == 0) {
    return exit_child(argc, argv);
  } else if (argc == 2 && strcmp(argv[1], "pipes") == 0) {
    return pipes_child(argc, argv);
  } else if (argc == 4 && strcmp(argv[1], "cloexec_check") == 0) {
    return cloexec_check_child(argc, argv);
  } else if (argc == 2 && strcmp(argv[1], "echo") == 0) {
    char msg[] = "test";
    write(1, msg, sizeof(msg));
    return 0;
  }
  // Preserve argv[0] for use in some tests.
  argv0 = argv[0];
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
