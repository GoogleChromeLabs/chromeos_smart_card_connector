/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// TODO(sbc): Remove this once html5f becomes the default for nacl-spawn
NaClProcessManager.fsroot = '/'

function run(nmf, cmd) {
  var mgr = new NaClProcessManager();
  // Assume a default terminal size for headless processes.
  mgr.onTerminalResize(80, 24);
  return new Promise(function(resolve, reject) {
    mgr.spawn(
        nmf, cmd, [], '/tmp', 'pnacl', null,
        function(pid) {
          mgr.waitpid(pid, 0, function(pid, code) { resolve(code); });
        });
  });
}

function runOk(nmf, cmd) {
  return run(nmf, cmd).then(function(result) {
    ASSERT_EQ(0, result);
  });
}

function getFile(filename) {
  return chrometest.httpGet(
      'filesystem:' + location.origin + '/temporary/' + filename);
}

TEST_F(chrometest.Test, 'testJSEvalString', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return runOk('jseval.nmf', ['jseval', '-e', '6 * 7', 'foo.txt']);
  }).then(function(result) {
    return getFile('foo.txt');
  }).then(function(data) {
    ASSERT_EQ('42', data);
  });
});

TEST_F(chrometest.Test, 'testJSEvalFile', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return runOk('jseval.nmf', ['jseval', '-e', '"9 * 9"', 'foo.txt']);
  }).then(function() {
    return runOk('jseval.nmf', ['jseval', '-f', 'foo.txt', 'bar.txt']);
  }).then(function() {
    return getFile('bar.txt');
  }).then(function(data) {
    ASSERT_EQ('81', data);
  });
});
