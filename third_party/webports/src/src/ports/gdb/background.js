/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// The background page of the gdb chrome packaged app exists solely to
// manage the set of operations that can only be performed in a v2 chrome app,
// namely those requiring sockets access.
//
// runGdb - Create a nacl module for gdb and marshall its messages.
// rspContinue - Allow a nacl module to run, waiting for it to exit or fault.
// rspDetach - Allow a nacl module to run without monitoring it.
// rspKill - Kill a nacl module by way of its debug port.


/**
 * Compute the checksum of a remote stub command.
 * @param {string} cmd Command to checksum.
 * @return {string} Two digit hex checksum of command.
 */
function rspChecksum_(cmd) {
  var checksum = 0;
  for (var i = 0; i < cmd.length; i++) {
    checksum = (checksum + cmd.charCodeAt(i)) & 0xff;
  }
  var checksumStr = '0' + checksum.toString(16);
  checksumStr = checksumStr.substr(checksumStr.length - 2);
  return checksumStr;
}

/**
 * Convert a string to an ArrayBuffer.
 * @param {string} str.
 * @return {ArrayBuffer}.
 */
function stringToArrayBuffer_(str) {
  var buf = new ArrayBuffer(str.length);
  var view = new Uint8Array(buf);
  for (var i = 0; i < str.length; i++) {
    view[i] = str.charCodeAt(i);
  }
  return buf;
}

/**
 * Add markers and checksum to a remote stub command.
 * @param {string} cmd Command to checksum.
 * @return {string} A ready to send checksumed remote stub message string.
 */
function rspChecksumMessage_(cmd) {
  return '$' + cmd + '#' + rspChecksum_(cmd);
}

/**
 * Add markers and checksum to a remote stub command.
 * @param {string} cmd Command to checksum.
 * @return {string} A ready to send checksumed remote stub message ArrayBuffer.
 */
function rspMessage_(cmd) {
  return stringToArrayBuffer_(rspChecksumMessage_(cmd));
}

/**
 * Decode a remote stub message.
 * @param {string} msg Message to decode.
 * @return {string} The decoded text.
 */
function rspDecode_(msg) {
  var view = new Uint8Array(msg);
  var result = '';
  for (var i = 0; i < msg.byteLength; i++) {
    result += String.fromCharCode(view[i]);
  }
  return result;
}

/**
 * Manage a single debug connection.
 * @private
 * @constructor
 */
function DebugConnection_(port) {
  this.port_ = port;
  this.debugTcpPort_ = null;
  this.socketId_ = null;
  this.gdbEmbed_ = null;

  port.onMessage.addListener(this.handleMessage_.bind(this));
  port.onDisconnect.addListener(this.handleDisconnect_.bind(this));
}

/**
 * Remove the gdb embed if any.
 */
DebugConnection_.prototype.removeEmbed_ = function() {
  if (this.gdbEmbed_ !== null) {
    var p = this.gdbEmbed_.parentNode;
    // Remove the embed from the parent if there is any.
    if (p !== null) {
      p.removeChild(this.gdbEmbed_);
    }
    this.gdbEmbed_ = null;
  }
};

/**
 * Disconnect connection to the debug stub if any.
 */
DebugConnection_.prototype.rspDisconnect_ = function() {
  if (this.socketId_ !== null) {
    chrome.socket.disconnect(this.socketId_);
    chrome.socket.destroy(this.socketId_);
    this.socketId_ = null;
  }
  this.removeEmbed_();
};

/**
 * Set the debug stub port.
 * @param {integer} debugTcpPort.
 */
DebugConnection_.prototype.setDebugTcpPort_ = function(debugTcpPort) {
  this.debugTcpPort_ = debugTcpPort;
};

/**
 * Connect to the debug stub.
 * @param {function()} callback.
 */
DebugConnection_.prototype.rspConnect_ = function(callback) {
  var self = this;
  self.rspDisconnect_();
  chrome.socket.create('tcp', function(createInfo) {
    self.socketId_ = createInfo.socketId;
    chrome.socket.connect(
      self.socketId_, '0.0.0.0', self.debugTcpPort_, function(result) {
        callback();
      });
  });
};

/**
 * Write a command to the debug stub.
 * @param {string} cmd Command to send.
 * @param {function()} callback.
 */
DebugConnection_.prototype.rspSendMessage_ = function(cmd, callback) {
  var msg = rspMessage_(cmd);
  chrome.socket.write(this.socketId_, msg, function(writeInfo) {
    callback();
  });
};

/**
 * Send gdb an interrupt.
 * @param {function()} callback.
 */
DebugConnection_.prototype.rspSendInterrupt_ = function(callback) {
  chrome.socket.write(
      this.socketId_, stringToArrayBuffer_('\x03'), function(writeInfo) {
    callback();
  });
};

/**
 * Connect and send a single command to the debug stub and hangup.
 * @param {string} cmd Command to send.
 * @param {function()} callback.
 */
DebugConnection_.prototype.rspOneCommand_ = function(cmd, callback) {
  var self = this;
  self.rspConnect_(function() {
    self.rspSendMessage_(cmd, function() {
      setTimeout(function() {
        self.rspDisconnect_();
        callback();
      }, 100);
    });
  });
};

/**
 * Wait for a the reply from the debug stub.
 * @param {function(Object)} callback Called on completion with an object
 *                                    containing the decoded and raw message.
 */
DebugConnection_.prototype.rspWait_ = function(callback) {
  var self = this;
  chrome.socket.read(self.socketId_, 1024, function(readInfo) {
    if (readInfo.resultCode > 0) {
      var rawReply = rspDecode_(readInfo.data);
      // The stub sends '+' signs to ack packets.
      // Ignore them as we're relying on TCP to ensure valid transmission.
      if (rawReply === '+') {
        self.rspWait_(callback);
      } else {
        // Messages are sent in the form '$' + message + '#' + checksum.
        // decode the message by slicing out just the message portion.
        var decoded = rawReply.slice(1, rawReply.length - 3);
        // Re-encode the message (adding '$' and '#' + checksum).
        var recoded = rspChecksumMessage_(decoded);
        // Verify the the message is still matches.
        if (recoded !== rawReply) {
          // Since there is a mismatch the message in invalid.
          // Sent a null for the decodedMessage.
          decoded = null;
        }
        callback({
          'reply': decoded,
          'rawReply': rawReply,
        });
      }
    } else {
      self.rspWait_(callback);
    }
  });
};

/**
 * Issue a continue command and wait for a fault.
 * @param {function(Object)} callback Called on completion with a partial
 *                                    decoding of the reply in an
 *                                    rspContinueReply message.
 */
DebugConnection_.prototype.rspContinue_ = function(callback) {
  var self = this;
  self.rspConnect_(function() {
    self.rspSendMessage_('vCont;c', function() {
      self.rspWait_(function(msg) {
        self.rspDisconnect_();
        msg ['name'] = 'rspContinueReply';
        if (msg.reply === null) {
          msg['type'] = 'invalid';
          msg['reply'] = 'INVALID: ' + msg.rawReply;
          callback(msg);
          return;
        }
        var first = msg.reply.slice(0, 1);
        if (['S', 'T', 'W', 'X'].indexOf(first) >= 0) {
          try {
            msg['number'] = parseInt(msg.reply.slice(1, 3), 16);
            msg['type'] = {
              'S': 'signal',
              'T': 'signal',
              'W': 'exited',
              'X': 'terminated',
            }[first];
          } catch (e) {
            msg['type'] = 'invalid';
          }
        } else {
          msg['type'] = 'unexpected';
        }
        callback(msg);
      });
    });
  });
};

/**
 * Issue a kill command.
 * @param {function()} callback.
 */
DebugConnection_.prototype.rspKill_ = function(callback) {
  this.rspOneCommand_('k', callback);
};

/**
 * Issue a detach command.
 * @param {function()} callback.
 */
DebugConnection_.prototype.rspDetach_ = function(callback) {
  this.rspOneCommand_('D', callback);
};

/**
 * Create a new GDB embed.
 */
DebugConnection_.prototype.createGdb_ = function() {
  var self = this;

  // TODO(bradnelson): Figured out why this does not work with an embed an
  // switch it.
  var gdb = document.createElement('object');
  self.gdbEmbed_ = gdb;
  gdb.width = 0;
  gdb.height = 0;
  gdb.type = 'application/x-nacl';
  gdb.data = 'gdb.nmf';

  gdb.addEventListener('message', function(e) {
  if (!self.port_) {
      return;
    }
    if (e.data.indexOf('exited:') === 0) {
      self.port_.postMessage(
          {'name': 'exited',
           'returncode': parseInt(e.data.split(':')[1])});
      gdb.removeEmbed_();
      return;
    }
    self.port_.postMessage({'name': 'message', 'data': e.data});
  });
  gdb.addEventListener('load', function(e) {
    if (!self.port_) {
      return;
    }
    self.port_.postMessage({'name': 'load'});
  });
  gdb.addEventListener('error', function(e) {
    if (!self.port_) {
      return;
    }
    self.port_.postMessage({'name': 'error'});
  });
  gdb.addEventListener('abort', function(e) {
    if (!self.port_) {
      return;
    }
    self.port_.postMessage({'name': 'abort'});
  });
  gdb.addEventListener('crash', function(e) {
    if (!self.port_) {
      return;
    }
    self.port_.postMessage({'name': 'crash'});
  });

  var params = {};
  params['PS_TTY_PREFIX'] = 'gdb';
  params['PS_TTY_RESIZE'] = 'tty_resize';
  params['PS_STDIN'] = '/dev/tty';
  params['PS_STDOUT'] = '/dev/tty';
  params['PS_STDERR'] = '/dev/tty';
  // TODO(bradnelson): Sort out how to cope with higher verbosity here.
  params['PS_VERBOSITY'] = '0';
  params['PS_EXIT_MESSAGE'] = 'exited';
  params['TERM'] = 'xterm-256color';
  params['PWD'] = '/';

  function addParam(name, value) {
    var param = document.createElement('param');
    param.name = name;
    param.value = value;
    gdb.appendChild(param);
  }

  for (var key in params) {
    addParam(key, params[key]);
  }

  var argv = [
      '-ex', 'target remote :' + self.debugTcpPort_,
  ];
  argv = ['gdb.nmf'].concat(argv);

  var argn = 0;
  argv.forEach(function(arg) {
    var argname = 'arg' + argn;
    addParam(argname, arg);
    argn = argn + 1
  });

  document.body.appendChild(gdb);
  // Work around crbug.com/350445
  var junk = gdb.offsetTop;
};

 /**
 * Suspend with javascript, then attach a GDB instance.
 */
DebugConnection_.prototype.runGdb_ = function() {
  var self = this;
  if (self.socketId_ === null) {
    // If we're not connected currently. Connect now.
    self.rspConnect_(function() {
      self.runGdb_();
    });
    return;
  }
  // Break the connection so GDB can attach.
  self.rspSendInterrupt_(function() {
    self.rspDisconnect_();
    self.createGdb_();
  });
};

/**
 * Handle a message from port.
 * @param msg
 */
DebugConnection_.prototype.handleMessage_ = function(msg) {
  var self = this;
  if (msg.name === 'input') {
    if (self.gdbEmbed_ !== null) {
      self.gdbEmbed_.postMessage(msg.msg);
    }
  } else if (msg.name === 'setDebugTcpPort') {
    self.setDebugTcpPort_(msg.debugTcpPort);
    self.port_.postMessage({'name': 'setDebugTcpPortReply'});
  } else if (msg.name === 'runGdb') {
    self.runGdb_();
  } else if (msg.name === 'rspKill') {
    self.rspKill_(function() {
      self.port_.postMessage({'name': 'rspKillReply'});
    });
  } else if (msg.name === 'rspDetach') {
    self.rspDetach_(function() {
      self.port_.postMessage({'name': 'rspDetachReply'});
    });
  } else if (msg.name === 'rspContinue') {
    self.rspContinue_(function(reply) {
      self.port_.postMessage(reply);
    });
  } else if (msg.name === 'installCheck') {
    self.port_.postMessage({'name': 'installCheckReply'});
  }
};

/**
 * Handle a disconnect from port.
 */
DebugConnection_.prototype.handleDisconnect_ = function() {
  this.rspDisconnect_();
  this.port_ = null;
};

/**
 * Create a new DebugConnection for each external connection.
 */
chrome.runtime.onConnectExternal.addListener(function(port) {
  // Check the sender only when not in testing mode.
  if (navigator.userAgent.indexOf('ChromeTestAgent/') < 0) {
    // TODO(bradnelson): Enable this check once a production extension id is
    // known for the debugger extension.
    //if (port.sender.id !== 'blessed-id') {
    //  port.disconnect();
    //  return;
    //}
  }
  var dc = new DebugConnection_(port);
});
