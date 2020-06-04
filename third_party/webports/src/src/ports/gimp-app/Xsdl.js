/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

NaClTerm.nmf = 'Xsdl.nmf';
NaClTerm.argv = [
  '-screen', '1140x810x32', '-ac', '-br', '-noreset', ':42'
];
NaClTerm.env = [
  'NACL_SPAWN_MODE=embed',
  'NACL_EMBED_WIDTH=100%',
  'NACL_EMBED_HEIGHT=100%',
];
document.title = 'Loading ... (this may take a while)';

var mgr = new NaClProcessManager();
mgr.onTerminalResize(80, 24);
var env = [
  'DISPLAY=:42',
];

// TODO(bradnelson): Do something more robust than racing gtk + xserver.
function startDemo() {
  // Assume a default terminal size for headless processes.
  mgr.spawn(
      'gimp.nmf', [  '-f','--no-shm', '--display=:42','--no-cpu-accel',
      '--new-instance',], env, '/', 'nacl', null, function(pid) {
    mgr.waitpid(pid, 0, function() {
      window.close();
    });
  });
  window.setTimeout(function(){document.title = 'Gimp'}, 25000);
}

function startWindowManager() {
  mgr.spawn(
      'twm.nmf', [""], env,
      '/', 'nacl', null, function(pid) {
    mgr.waitpid(pid, 0, function() {
      window.close();
    });
  });
}

startDemo();
window.setTimeout(function() {startWindowManager()}, 5000);
