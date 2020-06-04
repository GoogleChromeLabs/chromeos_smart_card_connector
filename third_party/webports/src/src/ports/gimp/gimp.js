/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

NaClTerm.nmf = 'gimp.nmf';
NaClTerm.env = [
  'NACL_SPAWN_MODE=embed',
  'NACL_EMBED_WIDTH=100%',
  'NACL_EMBED_HEIGHT=100%',
  'DISPLAY=:42',
];
NaClTerm.argv = [
  '-f',
  '--no-shm',
  '--display=:42',
  '--no-cpu-accel',
  '--new-instance',
];
document.title = 'Gimp';
