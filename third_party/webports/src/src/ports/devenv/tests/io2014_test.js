/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// Tests of the Google I/O 2014 tutorial at:
// https://developer.chrome.com/native-client/io2014

// Install the default packages.
// This test must be run before any tests that use these packages.
TEST_F(DevEnvTest, 'testDefaultPackageInstall', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return self.installDefaultPackages();
  });
});

TEST_F(DevEnvTest, 'testDemo', function() {
  // Test cat and rm.
  var self = this;
  var pid;
  var bashrc = 'git config --global user.name "John Doe"\n' +
               'git config --global user.email johndoe@example.com\n';
  var patch =
      'diff --git a/voronoi.cc b/voronoi.cc\n' +
      'index 897ec35..6e0b080 100644\n' +
      '--- a/voronoi.cc\n' +
      '+++ b/voronoi.cc\n' +
      '@@ -503,7 +503,7 @@ void Voronoi::Update() {\n' +
      '   if (NULL == ps_context_->data)\n' +
      '     return;\n' +
      '   assert(is_pow2(ps_context_->width));\n' +
      '-  assert(is_pow2(ps_context_->hieght));\n' +
      '+  assert(is_pow2(ps_context_->height));\n' +
      ' \n' +
      "   // When benchmarking is running, don't update display via\n" +
      '   // PSContext2DSwapBuffer() - vsync is enabled by default,' +
      ' and will throttle\n';
  return Promise.resolve().then(function() {
    return self.initFileSystem();
  }).then(function() {
    return self.writeFile('/home/user/.bashrc', bashrc);
  }).then(function() {
    return self.checkCommand('source ~/.bashrc');
  }).then(function() {
    return self.checkCommand('mkdir work');
  }).then(function() {
    return self.checkCommand(
        'cd work && ' +
        'curl http://nacltools.storage.googleapis.com/io2014/voronoi.zip -O');
  }).then(function() {
    return self.checkCommand('cd work && ls', 0, 'voronoi.zip\n');
  }).then(function() {
    return self.checkCommand('cd work && unzip voronoi.zip');
  }).then(function() {
    return self.checkCommand('cd work/voronoi && ls Makefile', 0, 'Makefile\n');
  }).then(function() {
    return self.checkCommand('cd work/voronoi && git init');
  }).then(function() {
    return self.checkCommand('cd work/voronoi && git add .');
  }).then(function() {
    return self.checkCommand(
        'cd work/voronoi && git commit -m "imported voronoi demo"');
  }).then(function() {
    return self.checkCommand('cd work/voronoi && make', 2);
  }).then(function() {
    return self.writeFile('/home/user/patch1', patch);
  }).then(function() {
    return self.checkCommand('cd work/voronoi && git apply ~/patch1');
  }).then(function() {
    return self.checkCommand('cd work/voronoi && make -j10');
  }).then(function() {
    return self.checkCommand(
        'cd work/voronoi && git commit -am "fixed build error"');
  }).then(function() {
    var sysArch = self.params['SYS_ARCH'];
    if (sysArch === 'i686') {
      var libDir = 'lib32';
      var suffix = 'x86_32';
    } else if (sysArch === 'x86_64') {
      var libDir = 'lib64';
      var suffix = 'x86_64';
    } else {
      ASSERT_TRUE(false, 'unknown arch: ' + sysArch);
    }
    return self.spawnCommand(
        'cd work/voronoi && ' +
        'LD_LIBRARY_PATH=' + libDir + ' ' +
        'NACL_SPAWN_MODE=popup ' +
        'NACL_POPUP_WIDTH=512 ' +
        'NACL_POPUP_HEIGHT=512 ' +
        './voronoi_' + suffix + '.nexe');
  }).then(function(msg) {
    pid = msg.pid;
    return chrometest.sleep(1000);
  }).then(function(msg) {
    self.sigint();
    return self.waitCommand(pid);
  }).then(function(msg) {
    ASSERT_EQ(128 + 9, msg.status, 'Expect kill status');
  });
});
