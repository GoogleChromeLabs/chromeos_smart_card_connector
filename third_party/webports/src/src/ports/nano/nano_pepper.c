/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "nacl_main.h"

extern int nacl_nano_main(int argc, char* argv[]);

int nacl_main(int argc, char* argv[]) {
  if (nacl_startup_untar(argv[0], "nano.tar", "/"))
    return -1;
  return nacl_nano_main(argc, argv);
}
