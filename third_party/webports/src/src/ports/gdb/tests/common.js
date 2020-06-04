/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

/**
 * Wait until a certain number of NaCl modules start/stop.
 * Waits until the number of extra modules vs a snapshot equals a certain
 * number. Also, waits until the debug port is known.
 * @param {integer} count Number of modules in addition to the snapshot to
 *     wait for.
 * @param {Object.<integer, ProcessInfo>} snapshot A snapshot of the the
 *     process set from getAllProcesses.
 * @return {Promise.Array.<ProcesssInfo>} Promise to wait until
 *     the module count matches with a list of extra modules.
 */
function waitForExtraModuleCount(count, snapshot) {
  return chrometest.getAllProcesses().then(function(processes) {
    var extraModules = [];
    for (var i in processes) {
      if (processes[i].type != 'nacl') {
        continue;
      }
      // Ignore modules until they have a known debug port.
      if (processes[i].naclDebugPort < 0) {
        continue;
      }
      if (!(i in snapshot)) {
        extraModules.push(processes[i]);
      }
    }
    if (extraModules.length == count) {
      return Promise.resolve(extraModules);
    } else {
      // Try again in 100ms.
      return chrometest.sleep(100).then(function() {
        return waitForExtraModuleCount(count, snapshot);
      });
    }
  });
}

/**
 * Create a new test module.
 * @return {Promise.[ProcessInfo, Element]} Promise to create element.
 */
function createModule() {
  var object = null;
  return chrometest.getAllProcesses().then(function(oldProcesses) {
    object = document.createElement('embed');
    object.setAttribute('src', 'test_module.nmf');
    object.setAttribute('type', 'application/x-nacl');
    object.setAttribute('width', '0');
    object.setAttribute('height', '0');
    document.body.appendChild(object);
    return waitForExtraModuleCount(1, oldProcesses);
  }).then(function(newModules) {
    return Promise.resolve([newModules[0], object]);
  });
}

/**
 * Create a waiter wrapper around a NaCl module.
 * @param {Element} module The module to wrap.
 * @return {chrometest.PortlikeWaiter} A promise based waiter.
 */
function moduleMessageWaiter(module) {
  var waiter;
  function handleMessage(msg) {
    waiter.enqueue(Promise.resolve(msg));
  }
  waiter = new chrometest.PortlikeWaiter(function() {
    module.removeEventListener(handleMessage);
  }, module);
  module.addEventListener('message', handleMessage, true);
  return waiter;
};


/**
 * Wait for a message other the name 'message' (from terminal).
 * @param {Port} portLike a port like waiter object to wait on.
 * @return {Promise} Promise to wait for a message.
 */
function waitIgnoringTerminal(portLike) {
  function waitForReply(msg) {
    // Ignore terminal messages.
    if (msg.name == 'message') {
      return portLike.wait().then(waitForReply);
    }
    return msg;
  }
  return portLike.wait().then(waitForReply);
}


function TestModuleTest() {
  chrometest.Test.call(this);
  this.process = null;
  this.object = null;
}
TestModuleTest.prototype = new chrometest.Test();

TestModuleTest.prototype.setUp = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return chrometest.Test.prototype.setUp.call(self);
  }).then(function() {
    // Snapshot the processes that are running before the test begins for later
    // use.
    return chrometest.getAllProcesses();
  }).then(function(initialProcesses) {
    self.initialProcesses = initialProcesses;
    return createModule();
  }).then(function(args) {
    self.process = args[0];
    self.rawObject = args[1];
    self.object = moduleMessageWaiter(args[1]);
    ASSERT_NE(null, self.process, 'there must be a process');
    // Done with ASSERT_FALSE because otherwise self.rawObject will attempt to
    // jsonify the DOM element.
    ASSERT_FALSE(null === self.rawObject, 'there must be a DOM element');
  });
};

TestModuleTest.prototype.tearDown = function() {
  var self = this;
  // Wait for the test module to exit.
  return waitForExtraModuleCount(0, self.initialProcesses).then(function() {
    // Remove node.
    if (self.object != null) {
      var object = self.object.detach();
      self.object = null;
      self.rawObject = null;
      var p = object.parentNode;
      p.removeChild(object);
    }
    return chrometest.Test.prototype.tearDown.call(self);
  });
};
