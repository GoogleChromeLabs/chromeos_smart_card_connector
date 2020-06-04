/* Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

#undef RUBY_EXPORT
#include <ruby.h>
#include <spawn.h>
#include <stdio.h>

#include "nacl_main.h"

#ifdef __x86_64__
#define NACL_ARCH "x86_64"
#elif defined __i386__
/*
 * Use __i386__ rather then __i686__ since the latter is not defined
 * by i686-nacl-clang.
 */
#define NACL_ARCH "x86_32"
#elif defined __arm__
#define NACL_ARCH "arm"
#elif defined __pnacl__
#define NACL_ARCH "pnacl"
#else
#error "unknown arch"
#endif

#define DATA_ARCHIVE "rbdata-" NACL_ARCH ".tar"

int nacl_main(int argc, char** argv) {
  if (nacl_startup_untar(argv[0], DATA_ARCHIVE, "/"))
    return -1;

  if (argc == 2 && !strcmp(argv[1], "/bin/irb"))
    fprintf(stderr, "Launching irb ...\n");
  ruby_sysinit(&argc, &argv);
  {
    RUBY_INIT_STACK;
    ruby_init();
    return ruby_run_node(ruby_options(argc, argv));
  }
}
