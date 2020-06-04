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

// TODO(bradnelson): Do something more robust than racing emacs + xserver.
function startEmacs() {
  var mgr = new NaClProcessManager();
  // Assume a default terminal size for headless processes.
  mgr.onTerminalResize(80, 24);
  var env = [
    'DISPLAY=:42',
  ];
  mgr.spawn(
      'emacs.nmf', ['-g', '140x53'], env,
      '/home/user', 'nacl', null, function(pid) {
    mgr.waitpid(pid, 0, function() {
      window.close();
    });
  });
}
startEmacs();

document.title = 'GNU Emacs';
