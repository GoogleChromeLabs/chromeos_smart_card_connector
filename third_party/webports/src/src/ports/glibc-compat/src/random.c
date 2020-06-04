/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>

/*
 * Implemenat random/srandom in terms of rand/srand.
 * TODO(sbc): remove this once these get added to newlib.
 */

long int random(void) __attribute__((weak));
long int random(void) {
  return rand();
}

void srandom(unsigned int seed) __attribute__((weak));
void srandom(unsigned int seed) {
  srand(seed);
}
