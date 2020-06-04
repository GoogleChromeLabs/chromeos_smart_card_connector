/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Define a typical entry point for command line tools spawned by bash
 * (e.g., ls, objdump, and objdump). */

#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "nacl_io/nacl_io.h"
#include "nacl_main.h"
#include "ppapi_simple/ps.h"
#include "ppapi_simple/ps_main.h"

#ifdef _NEWLIB_VERSION
void setprogname(const char *progname) __attribute__((weak));
#endif

void nacl_setprogname(char* argv0) {
#if defined(_NEWLIB_VERSION)
  /* If setprogname exists at runtime then call it to set the program name */
  if (setprogname)
    setprogname(argv0);
#elif defined(__GLIBC__)
  char *p = strrchr (argv0, '/');
  if (p == NULL)
    program_invocation_short_name = argv0;
  else
    program_invocation_short_name = p + 1;
  program_invocation_name = argv0;
#endif
}

int cli_main(int argc, char* argv[]) {
  if (argv && argv[0])
    nacl_setprogname(argv[0]);

  int rtn = nacl_setup_env();
  if (rtn != 0) {
    fprintf(stderr, "nacl_setup_env failed: %d\n", rtn);
    return 1;
  }
  return nacl_main(argc, argv);
}

PPAPI_SIMPLE_REGISTER_MAIN(cli_main)
