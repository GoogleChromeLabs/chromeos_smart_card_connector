/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// A testing extension that grants ordinary pages access to extension specific
// functionality when run in test mode.


/**
 * The list of all current connections.
 * @private
 */
var g_connections_ = [];


/**
 * Kill the browser.
 */
function haltBrowser() {
  chrome.processes.getProcessInfo([], false, function(processes) {
    for (var p in processes) {
      if (processes[p].type === 'browser') {
        chrome.processes.terminate(processes[p].id);
      }
    }
  });
  // This seems to be needed in practice sometimes.
  chrome.processes.terminate(0);
}


/**
 * A connection.
 * @constructor
 * @param {Port} port for the connection.
 */
function Connection(port) {
  this.proxied_ = null;
  this.port_ = port;
  this.port_.onMessage.addListener(this.handleMessage_.bind(this));
  this.port_.onDisconnect.addListener(this.handleDisconnect_.bind(this));
  g_connections_.push(this);
}

/**
 * Disconnect all ports related to the connection.
 */
Connection.prototype.disconnect = function() {
  try {
    this.port_.disconnect();
  } catch (e) {
  }
  if (this.proxied_ !== null) {
    try {
      this.proxied_.disconnect();
    } catch (e) {
    }
  }
  for (var i = 0; i < g_connections_.length; i++) {
    if (g_connections_[i] === this) {
      g_connections_.splice(i, 1);
      break;
    }
  }
};

/**
 * Handle a message from the primary connection port.
 * @param {Message} msg The inbound message.
 */
Connection.prototype.handleMessage_ = function(msg) {
  var self = this;
  // When in proxy mode, route all messages.
  if (self.proxied_ !== null) {
    self.proxied_.postMessage(msg);

  // Expose a means to halt the entire browser.
  } else if (msg.name == 'haltBrowser') {
    haltBrowser();

  // Expose chrome.management.getAll.
  } else if (msg.name == 'getAllExtensions') {
    chrome.management.getAll(function(result) {
       self.port_.postMessage({'name': 'getAllExtensionsResult',
                               'result': result});
    });

  // Expose chrome.processes.getProcessInfo.
  } else if (msg.name == 'getAllProcesses') {
    // TODO(bradnelson): Remove this when chrome is fixed.
    // Currently the task manager does not pump out notification of updates to
    // the chrome.processes api when there are no pending listeners.
    // Oddly the listeners seem to need to be started after a getProcessInfo
    // request.
    var doNothing = function(processes) {};
    chrome.processes.getProcessInfo([], false, function(processes) {
      self.port_.postMessage({'name': 'getAllProcessesResult',
                              'result': processes});
    // TODO(bradnelson): Remove this when chrome is fixed. See above.
      chrome.processes.onUpdated.removeListener(doNothing);
    });
    // TODO(bradnelson): Remove this when chrome is fixed. See above.
    chrome.processes.onUpdated.addListener(doNothing);

  // Allow proxied access to all extensions / apps for testing.
  // NOTE: Once you switch to proxy mode, all messages are routed to the
  // proxied extension. A new connection is required for further access to
  // other functionality.
  } else if (msg.name == 'proxy') {
    chrome.management.getAll(function(extensions) {
      // Find all extensions with this name.
      var picks = [];
      for (var i = 0; i < extensions.length; i++) {
        if (extensions[i].name == msg.extension) {
          picks.push(extensions[i]);
        }
      }
      var success = false;
      if (picks.length == 1) {
        self.proxied_ = chrome.runtime.connect(picks[0].id);
        self.proxied_.onMessage.addListener(function(msg) {
          self.port_.postMessage(msg);
        });
        self.proxied_.onDisconnect.addListener(function(msg) {
          self.disconnect();
        });
        success = true;
      }
      // Reply in any event.
      self.port_.postMessage({'name': 'proxyReply', 'success': success,
                              'matchCount': picks.length});
    });

  // Provide a simple echo for testing of this extension.
  } else if (msg.name == 'ping') {
    msg.name = 'pong';
    self.port_.postMessage(msg);

  // Disconnect all connections except the current one.
  } else if (msg.name == 'reset') {
    var snapshot = g_connections_.slice();
    var killCount = 0;
    for (var i = 0; i < snapshot.length; i++) {
      // Disconnect everything other than the current connection.
      if (snapshot[i] !== self) {
        snapshot[i].disconnect();
        killCount++;
      }
    }
    self.port_.postMessage({'name': 'resetReply', 'count': killCount});
  }
};

/**
 * Handle disconnect from the primary connection prot.
 */
Connection.prototype.handleDisconnect_ = function() {
  this.disconnect();
};


/**
 * Listen for a number of message types from tests.
 */
chrome.runtime.onConnectExternal.addListener(function(port) {
  var connection = new Connection(port);
});
