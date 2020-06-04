/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

function GdbExtensionTestModuleTest() {
  TestModuleTest.call(this);
  this.gdbExt = null;
}
GdbExtensionTestModuleTest.prototype = new TestModuleTest();

GdbExtensionTestModuleTest.prototype.setUp = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return TestModuleTest.prototype.setUp.call(self);
  }).then(function() {
    return self.newGdbExtPort(self.process.naclDebugPort);
  }).then(function(gdbExt) {
    self.gdbExt = gdbExt;
  });
};

GdbExtensionTestModuleTest.prototype.tearDown = function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.gdbExt.disconnect();
    return TestModuleTest.prototype.tearDown.call(self);
  });
};

/**
 * Create a new port to the GDB extension.
 * @param {integer} debugTcpPort Tcp port of the module to manage.
 * @return {Promise.Port} Promise a port to the GDB extension.
 */
GdbExtensionTestModuleTest.prototype.newGdbExtPort = function(debugTcpPort) {
  var keepPort = null;
  return Promise.resolve().then(function() {
    return chrometest.proxyExtension('Native Client GDB');
  }).then(function(gdbExtPort) {
    keepPort = gdbExtPort;
    keepPort.postMessage({'name': 'setDebugTcpPort',
                          'debugTcpPort': debugTcpPort});
    return keepPort.wait();
  }).then(function(msg) {
    ASSERT_EQ('setDebugTcpPortReply', msg.name,
      'expect debug extension port reply');
    return Promise.resolve(keepPort);
  });
};

/**
 * Start up GDB and detach.
 * @return {Promise} A promise to wait.
 */
GdbExtensionTestModuleTest.prototype.runGdb = function() {
  var self = this;
  var keepPort = null;
  return Promise.resolve().then(function() {
    return chrometest.getAllProcesses();
  }).then(function(oldProcesses) {
    // Start gdb on the target process.
    self.gdbExt.postMessage({'name': 'runGdb'});
    return waitForExtraModuleCount(1, oldProcesses);
  }).then(function(newModules) {
    var gdbProcess = newModules[0];
    return self.newGdbExtPort(gdbProcess.naclDebugPort);
  }).then(function(gdbExtForGdb) {
    keepPort = gdbExtForGdb;
    keepPort.postMessage({'name': 'rspDetach'});
    return keepPort.wait();
  }).then(function(msg) {
    ASSERT_EQ('rspDetachReply', msg.name, 'expect successful detach');
    keepPort.disconnect();
    // Wait for load.
    return self.gdbExt.wait();
  }).then(function(msg) {
    EXPECT_EQ('load', msg.name, 'expecting a load');
    // Expecting 16-19 lines out.
    return self.waitForGdbPrompt(16, 19);
  });
};

/**
 * Wait for the gdb prompt.
 *
 * Expect variable (within a range) amount of output from GDB prior to the
 * "(gdb)" prompt returning.
 * @param {integer} min Minimum number of writes (lines plus echo) expected
 *                      from GDB.
 * @param {integer} max Maximum number of writes (lines plus echo) expected
 *                      from GDB.
 * @return {Promise} A promise to wait.
 */
GdbExtensionTestModuleTest.prototype.waitForGdbPrompt = function(min, max) {
  var self = this;
  var lineCount = 0;
  function checkLine(msg) {
    ASSERT_EQ('message', msg.name, 'expecting a message');
    var prefix = msg.data.slice(0, 3);
    var data = msg.data.slice(3);
    EXPECT_EQ('gdb', prefix, 'expected gdb term message');
    if (data == '(gdb) ') {
      ASSERT_GE(lineCount, min, 'expect gdb to emit more lines first');
      // Done.
      return;
    } else {
      lineCount++;
      ASSERT_LE(lineCount, max, 'expect limited text, got extra: ' + data);
      // Recurse.
      return self.gdbExt.wait().then(checkLine);
    }
  }
  return self.gdbExt.wait().then(checkLine);
};

/**
 * Wait for expected writes from gdb.
 * @param {Array.String} writes A list of writes to expect from GDB.
 * @return {Promise} A promise to wait.
 */
GdbExtensionTestModuleTest.prototype.waitForGdbReply = function(lines) {
  var self = this;
  function checkLine() {
    if (lines.length == 0) {
      return;
    }
    return Promise.resolve().then(function() {
      return self.gdbExt.wait();
    }).then(function(msg) {
      ASSERT_EQ('message', msg.name, 'expecting a message');
      var prefix = msg.data.slice(0, 3);
      var data = msg.data.slice(3);
      EXPECT_EQ('gdb', prefix, 'expected gdb term message');
      EXPECT_EQ(lines[0], data, 'expect matching line');
      lines = lines.slice(1);
      return checkLine();
    });
  }
  return checkLine();
};

/**
 * Send a command to gdb (as keystrokes).
 * @param {String} cmd.
 */
GdbExtensionTestModuleTest.prototype.sendGdbCommand = function(cmd) {
  var self = this;
  self.gdbExt.postMessage(
      {'name': 'input', 'msg': {'gdb': cmd}});
};

/**
 * Send a command to gdb and wait for echo followed by a reply.
 *
 * Checks for (gdb) prompt after the command.
 * @param {String} cmd The command to send.
 * @param {Array.String} expected A list of expected write back from gdb.
 * @return {Promise} A promise to do it.
 */
GdbExtensionTestModuleTest.prototype.gdbCommand = function(cmd, expected) {
  var self = this;
  return Promise.resolve().then(function() {
    self.sendGdbCommand(cmd);
    var response = [];
    for (var i = 0; i < cmd.length; i++) {
      response.push(cmd[i]);
    }
    response = response.concat(expected);
    response.push('(gdb) ');
    return self.waitForGdbReply(response);
  });
};

/**
 * Exit GDB and check for expected behavior.
 * @return {Promise} A promise to do it.
 */
GdbExtensionTestModuleTest.prototype.exitGdb = function() {
  var self = this;
  return Promise.resolve().then(function() {
    var msg = 'kill\ny\n';
    self.sendGdbCommand(msg);
    // The kill y/n prompt comes back with one less write than the number of
    // character for some reason.
    var len = msg.length - 1;
    return self.waitForGdbPrompt(len, len);
  }).then(function() {
    self.sendGdbCommand('quit\n');
    return waitIgnoringTerminal(self.gdbExt);
  }).then(function(msg) {
    EXPECT_EQ('exited', msg.name, 'expect exit');
    EXPECT_EQ(0, msg.returncode, 'expect 0');
  });
};


TEST_F(GdbExtensionTestModuleTest, 'testRspKill', function() {
  var self = this;
  self.gdbExt.postMessage({'name': 'rspKill'});
  return self.gdbExt.wait().then(function(msg) {
    EXPECT_EQ('rspKillReply', msg.name, 'reply should be right');
  });
});


TEST_F(GdbExtensionTestModuleTest, 'testRspDetach', function() {
  var self = this;
  self.gdbExt.postMessage({'name': 'rspDetach'});
  return self.gdbExt.wait().then(function(msg) {
    EXPECT_EQ('rspDetachReply', msg.name, 'reply should be right');
    // Wait a bit for the module to start.
    return chrometest.sleep(500);
  }).then(function() {
    self.object.postMessage('ping');
    return self.object.wait();
  }).then(function(msg) {
    EXPECT_EQ('pong', msg.data);
    self.object.postMessage('exit');
  });
});


TEST_F(GdbExtensionTestModuleTest, 'testRspContinueOk', function() {
  var self = this;
  self.gdbExt.postMessage({'name': 'rspContinue'});
  // Wait a bit for the module to start.
  return chrometest.sleep(500).then(function() {
    self.object.postMessage('ping');
    return self.object.wait();
  }).then(function(msg) {
    EXPECT_EQ('pong', msg.data);
    self.object.postMessage('exit');
    return self.gdbExt.wait();
  }).then(function(msg) {
    EXPECT_EQ('rspContinueReply', msg.name);
    EXPECT_EQ('exited', msg.type,
        'expected module exit but got: ' + msg.reply);
    EXPECT_EQ(0, msg.number, 'expected 0 exit code');
  });
});


TEST_F(GdbExtensionTestModuleTest, 'testRspContinueFault', function() {
  var self = this;
  self.gdbExt.postMessage({'name': 'rspContinue'});
  // Wait a bit for the module to start.
  return chrometest.sleep(500).then(function() {
    self.object.postMessage('ping');
    return self.object.wait();
  }).then(function(msg) {
    EXPECT_EQ('pong', msg.data);
    self.object.postMessage('fault');
    return self.gdbExt.wait();
  }).then(function(msg) {
    EXPECT_EQ('rspContinueReply', msg.name);
    EXPECT_EQ('signal', msg.type,
        'expected module signal but got: ' + msg.reply);
    self.gdbExt.postMessage({'name': 'rspKill'});
    return self.gdbExt.wait();
  }).then(function(msg) {
    EXPECT_EQ('rspKillReply', msg.name, 'reply should be right');
  });
});


TEST_F(GdbExtensionTestModuleTest, 'testRspContinueTwice', function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.gdbExt.postMessage({'name': 'rspContinue'});
    // Wait a bit for the module to start.
    return chrometest.sleep(500);
  }).then(function() {
    self.object.postMessage('ping');
    return self.object.wait();
  }).then(function(msg) {
    EXPECT_EQ('pong', msg.data);
    // Continue a second time (should disconnect and reconnect).
    self.gdbExt.postMessage({'name': 'rspContinue'});
    // Wait for it to start.
    return chrometest.sleep(500);
  }).then(function() {
    self.object.postMessage('ping');
    return self.object.wait();
  }).then(function(msg) {
    EXPECT_EQ('pong', msg.data);
    self.object.postMessage('exit');
    return self.gdbExt.wait();
  }).then(function(msg) {
    EXPECT_EQ('rspContinueReply', msg.name);
    EXPECT_EQ('exited', msg.type,
        'expected module exit but got: ' + msg.reply);
    EXPECT_EQ(0, msg.number, 'expected 0 exit code');
  });
});


TEST_F(GdbExtensionTestModuleTest, 'testGdbStart', function() {
  var self = this;
  return self.runGdb().then(function() {
    return self.exitGdb();
  });
});


TEST_F(GdbExtensionTestModuleTest, 'testGdbLoadSymbols', function() {
  var self = this;
  return self.runGdb().then(function() {
    return self.gdbCommand('remote get irt irt\n', [
        'Successfully fetched file "irt".\n',
    ]);
  }).then(function() {
    return self.gdbCommand('nacl-irt irt\n', [
        'Reading symbols from irt...',
        '(no debugging symbols found)...done.\n',
    ]);
  }).then(function()  {
    return self.gdbCommand('remote get nexe nexe\n', [
        'Successfully fetched file "nexe".\n',
    ]);
  }).then(function() {
    // Loads symbols from nexe without prompting.
    return self.gdbCommand('nacl-irt nexe\n', [
        'Reading symbols from nexe...',
        'done.\n',
    ]);
  }).then(function() {
    return self.exitGdb();
  });
});


TEST('GdbTest', 'testInstallCheck', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return chrometest.proxyExtension('Native Client GDB');
  }).then(function(gdbExt) {
    self.gdbExt = gdbExt;
    self.gdbExt.postMessage({name: 'installCheck'});
    return self.gdbExt.wait();
  }).then(function(msg) {
    EXPECT_EQ('installCheckReply', msg.name, 'we are installed');
    self.gdbExt.disconnect();
  });
});
