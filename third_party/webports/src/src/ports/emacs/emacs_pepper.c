/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// TODO(bradnelson): Find a better workaround.
#define NACL_SKIP_GETOPT

#include "nacl_main.h"

// Some things that Lisp.h needs:
#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/time.h>

#include "lisp.h"

// Add some functions for use in the debugger to print out the value
// of a lisp object.

// Print a human readable type for a Lisp object to the debug console.
char debug_print_buf[81];
char* whatis(Lisp_Object object) {
  debug_print_buf[0] = '\0';
  debug_print_buf[80] = '\0';

  if (STRINGP(object)) {
    snprintf(debug_print_buf, 80, "String %s", SSDATA(object));
    return debug_print_buf;
  } else if (INTEGERP(object)) {
    int x = XINT(object);
    snprintf(debug_print_buf, 80, "Number %d", x);
    return debug_print_buf;
  } else if (FLOATP(object)) {
    struct Lisp_Float* floater = XFLOAT(object);
    return "It's a float number!";
  } else if (Qnil == object)
    return "It's a lisp null";
  else if (Qt == object)
    return "It's a lisp 't'";
  else if (SYMBOLP(object)) {
    snprintf(debug_print_buf, 80, "Symbol named %s", SYMBOL_NAME(object));
    return debug_print_buf;
  } else if (CONSP(object))
    return "It's a list!";
  else if (MISCP(object))
    return "It's a lisp misc!";
  else if (VECTORLIKEP(object))
    return "It's some kind of vector like thingie!";
  else
    return "I don't know what it is.";
}

// The special NaCl entry point into emacs.
extern int nacl_emacs_main(int argc, char* argv[]);

int nacl_main(int argc, char* argv[]) {
  if (nacl_startup_untar(argv[0], "emacs.tar", "/"))
    return 1;
  return nacl_emacs_main(argc, argv);
}
