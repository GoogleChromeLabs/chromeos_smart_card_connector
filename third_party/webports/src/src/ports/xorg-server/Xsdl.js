/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

NaClTerm.nmf = 'Xsdl.nmf';
NaClTerm.argv = [
  '-screen', '1024x768x32', '-ac', '-br', '-noreset', ':42'
];
NaClTerm.env = [
  'NACL_SPAWN_MODE=embed',
  'NACL_EMBED_WIDTH=100%',
  'NACL_EMBED_HEIGHT=100%',
];
