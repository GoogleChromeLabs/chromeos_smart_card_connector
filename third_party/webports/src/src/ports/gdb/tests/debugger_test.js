/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

function DebugExtensionTest() {
  this.debugExt = null;
  this.moduleId = null;
  this.change = null;
}
DebugExtensionTest.prototype = new TestModuleTest();

DebugExtensionTest.prototype.setUp = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return TestModuleTest.prototype.setUp.call(self);
  }).then(function() {
    // Wait for test module to be run by extensions.
    return chrometest.sleep(500);
  }).then(function() {
    return chrometest.proxyExtension('Native Client Debugger');
  }).then(function(debugExt) {
    self.debugExt = debugExt;
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('join', msg.cause);
    var ids = Object.keys(msg.naclModules);
    ASSERT_EQ(1, ids.length);
    self.moduleId = ids[0];
    self.change = msg;
    self.initialSettings = JSON.parse(JSON.stringify(msg.settings));
    ASSERT_TRUE(msg.naclModules[self.moduleId].process.title.indexOf(
        'test_module.nmf') >= 0);
  });
};

DebugExtensionTest.prototype.tearDown = function() {
  var self = this;
  return Promise.resolve().then(function() {
    // Put extension back in default state.
    self.debugExt.postMessage({'name': 'defaultSettings'});
    // Also skip over remaining terminal messages.
    return waitIgnoringTerminal(self.debugExt);
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('settingsChange', msg.cause);
    // Confirm extension has been put back in default state.
    for (var key in self.initialSettings) {
      ASSERT_EQ(self.initialSettings[key], msg.settings[key],
          'check key: ' + key);
    }
    // Then disconnect from extension.
    self.debugExt.disconnect();
    return TestModuleTest.prototype.tearDown.call(self);
  });
};

DebugExtensionTest.prototype.checkAttach = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('faulted', msg.cause);
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    // In cases where the debug port is not immediately known,
    // a later 'updated' event may trigger attach instead of the initial
    // 'create'.
    ASSERT_TRUE('create' ==  msg.cause || 'updated' == msg.cause);
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('setup', msg.cause);
    self.debugExt.postMessage({name: 'tune', 'processId': self.moduleId});
    // Wait for output from gdb before interacting with it.
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('message', msg.name);
  });
};

DebugExtensionTest.prototype.checkQuitGdb = function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.debugExt.postMessage(
      {'name': 'input', 'msg': {'gdb': 'kill\ny\n'}});
    return waitIgnoringTerminal(self.debugExt);
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('exit', msg.cause);
    var ids = Object.keys(msg.naclModules);
    ASSERT_EQ(1, ids.length);
    self.debugExt.postMessage(
      {'name': 'input', 'msg': {'gdb': 'quit\n'}});
    return waitIgnoringTerminal(self.debugExt);
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('exit', msg.cause);
    var ids = Object.keys(msg.naclModules);
    ASSERT_EQ(0, ids.length);
  });
};


TEST_F(DebugExtensionTest, 'testExit', function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.object.postMessage('exit');
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('exit', msg.cause);
    var ids = Object.keys(msg.naclModules);
    ASSERT_EQ(0, ids.length);
  });
});


TEST_F(DebugExtensionTest, 'testFaultKilled', function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.change.settings.onFaultAttach = false;
    self.debugExt.postMessage(
        {'name': 'settingsChange', 'settings': self.change.settings});
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('settingsChange', msg.cause);
    ASSERT_FALSE(msg.settings.onFaultAttach);
    self.object.postMessage('fault');
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('killed', msg.cause);
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('exit', msg.cause);
    var ids = Object.keys(msg.naclModules);
    ASSERT_EQ(0, ids.length);
  });
});


TEST_F(DebugExtensionTest, 'testFaultAttach', function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.change.settings.onFaultAttach = true;
    self.debugExt.postMessage(
        {'name': 'settingsChange', 'settings': self.change.settings});
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('settingsChange', msg.cause);
    ASSERT_TRUE(msg.settings.onFaultAttach);
    self.object.postMessage('fault');
    return self.checkAttach();
  }).then(function() {
    return self.checkQuitGdb();
  });
});


TEST_F(DebugExtensionTest, 'testRunningKill', function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.debugExt.postMessage({name: 'kill', 'processId': self.moduleId});
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('exit', msg.cause);
    var ids = Object.keys(msg.naclModules);
    ASSERT_EQ(0, ids.length);
  });
});


TEST_F(DebugExtensionTest, 'testRunningAttach', function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.debugExt.postMessage({name: 'attach', 'processId': self.moduleId});
    self.debugExt.postMessage({name: 'tune', 'processId': self.moduleId});
    return self.checkAttach();
  }).then(function() {
    return self.checkQuitGdb();
  });
});


TEST('DebugExtensionTest', 'testInstallCheck', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return chrometest.proxyExtension('Native Client Debugger');
  }).then(function(debugExt) {
    self.debugExt = debugExt;
    return self.debugExt.wait();
  }).then(function(msg) {
    ASSERT_EQ('change', msg.name);
    ASSERT_EQ('join', msg.cause);
    self.debugExt.postMessage({name: 'installCheck'});
    return self.debugExt.wait();
  }).then(function(msg) {
    EXPECT_EQ('installCheckReply', msg.name, 'we are installed');
    self.debugExt.disconnect();
  });
});
