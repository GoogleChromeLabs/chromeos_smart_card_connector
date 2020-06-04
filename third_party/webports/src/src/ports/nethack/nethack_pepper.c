/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "nacl_main.h"

extern int nethack_main(int argc, char* argv[]);

static void setup_unix_environment(const char* arg0) {
  mkdir("/usr", 0777);
  mkdir("/usr/games", 0777);

  int ret = nacl_startup_untar(arg0, "nethack.tar", "/usr/games");
  if (ret != 0) {
    perror("Startup untar failed.");
    exit(1);
  }

  // Setup config file.
  ret = chdir(getenv("HOME"));
  if (ret != 0) {
    perror("Can change to HOME dir.");
    exit(1);
  }
  if (access(".nethackrc", R_OK) < 0) {
    int fh = open(".nethackrc", O_CREAT | O_WRONLY);
    const char config[] = "OPTIONS=color\n";
    write(fh, config, sizeof(config) - 1);
    close(fh);
  }

  ret = chdir("/usr/games");
  if (ret != 0) {
    perror("Can change to /usr/games.");
    exit(1);
  }
}

int nacl_main(int argc, char* argv[]) {
  setup_unix_environment(argv[0]);
  return nethack_main(argc, argv);
}
