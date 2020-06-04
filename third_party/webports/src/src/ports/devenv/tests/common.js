/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

function DevEnvTest() {
  chrometest.Test.call(this);
  this.devEnv = null;
  this.tcp = null;
  this.params = chrometest.getUrlParameters();

  // Buffer incoming TCP messages.
  this.tcpBuffer = {};
  this.tcpDisconnected = {};
};

DevEnvTest.prototype = new chrometest.Test();
DevEnvTest.prototype.constructor = DevEnvTest;

DevEnvTest.prototype.setUp = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return chrometest.Test.prototype.setUp.call(self);
  }).then(function() {
    return chrometest.proxyExtension('NaCl Development Environment');
  }).then(function(ext) {
    self.devEnv = ext;
  }).then(function() {
    return chrometest.proxyExtension('TCP Interface');
  }).then(function(ext) {
    self.tcp = ext;
    return self.initFileSystem();
  }).then(function() {
    return self.mkdir('/home');
  }).then(function() {
    return self.mkdir('/home/user');
  }).then(function() {
    return self.mkdir('/usr');
  }).then(function() {
    return self.mkdir('/usr/etc');
  }).then(function() {
    return self.mkdir('/usr/etc/pkg');
  }).then(function() {
    return self.mkdir('/usr/etc/pkg/repos');
  }).then(function() {
    if (self.params['latest'] === '1')
      return self.setRepo(window.location.origin + '/publish');
  }).then(function() {
    return self.mkdir('/home/user');
  });
};

DevEnvTest.prototype.tearDown = function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.disconnect();
    self.tcp.disconnect();
    return chrometest.Test.prototype.tearDown.call(self);
  });
};

DevEnvTest.prototype.waitWhile = function(ext, condition, body) {
  function loop(arg) {
    if (!condition(arg)) {
      return arg;
    } else {
      body(arg);
      return ext.wait().then(loop);
    }
  }
  return ext.wait().then(loop);
};

DevEnvTest.prototype.gatherStdoutUntil = function(name) {
  var self = this;
  var output = '';

  return Promise.resolve().then(function() {
    return self.waitWhile(self.devEnv,
      function condition(msg) { return msg.name !== name; },
      function body(msg) {
        ASSERT_EQ('nacl_stdout', msg.name);
        output += msg.output;
        chrometest.info('stdout: ' + msg.output);
      });
  }).then(function(msg) {
    ASSERT_EQ(name, msg.name, 'expected message');
    msg.output = output;
    return msg;
  });
};

DevEnvTest.prototype.spawnCommand = function(cmd, cmdPrefix) {
  var self = this;

  if (cmdPrefix === undefined) {
    cmdPrefix = '. /mnt/http/setup-environment && ';
  }

  var env = ['HOME=/home/user', 'NACL_DATA_MOUNT_FLAGS=manifest=/manifest.txt'];
  if (this.params['latest'] === '1') {
    env.push('NACL_DEVENV_LOCAL=1');
  }

  return Promise.resolve().then(function() {
    self.devEnv.postMessage({
      'name': 'nacl_spawn',
      'nmf': 'bash.nmf',
      'argv': ['bash', '-c', cmdPrefix + cmd],
      'cwd': '/home/user',
      'envs': env,
    });
    return self.gatherStdoutUntil('nacl_spawn_reply');
  }).then(function(msg) {
    ASSERT_EQ('nacl_spawn_reply', msg.name);
    return {pid: msg.pid, output: msg.output};
  });
};

DevEnvTest.prototype.waitCommand = function(pid) {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.postMessage({
      'name': 'nacl_waitpid',
      'pid': pid,
      'options': 0,
    });
    return self.gatherStdoutUntil('nacl_waitpid_reply');
  }).then(function(msg) {
    ASSERT_EQ('nacl_waitpid_reply', msg.name);
    ASSERT_EQ(pid, msg.pid);
    return {status: msg.status, output: msg.output};
  });
};

DevEnvTest.prototype.runCommand = function(cmd, cmdPrefix) {
  var self = this;
  var earlyOutput;
  chrometest.info('runCommand: ' + cmd);
  return Promise.resolve().then(function() {
    return self.spawnCommand(cmd, cmdPrefix);
  }).then(function(msg) {
    earlyOutput = msg.output;
    return self.waitCommand(msg.pid);
  }).then(function(msg) {
    msg.output = earlyOutput + msg.output;
    return msg;
  });
};

DevEnvTest.prototype.checkCommand = function(
    cmd, expectedStatus, expectedOutput) {
  if (expectedStatus === undefined) {
    expectedStatus = 0;
  }
  var self = this;
  return Promise.resolve().then(function() {
    return self.runCommand(cmd);
  }).then(function(result) {
    ASSERT_EQ(expectedStatus, result.status, result.output);
    if (expectedOutput !== undefined) {
      ASSERT_EQ(expectedOutput, result.output);
    }
  });
};

DevEnvTest.prototype.sigint = function(pid) {
  this.devEnv.postMessage({
    'name': 'nacl_sigint',
  });
};

DevEnvTest.prototype.installPackage = function(name) {
  var cmd = 'pkg install -f -y ' + name;
  chrometest.info(cmd);
  return this.checkCommand(cmd, 0);
};

DevEnvTest.prototype.installDefaultPackages = function(name) {
  var cmd = 'bash /mnt/http/install-base-packages.sh';
  chrometest.info(cmd);
  return this.checkCommand(cmd, 0);
};


DevEnvTest.prototype.checkCommandReLines = function(
    cmd, expectedStatus, expectedOutput) {
  var self = this;
  return Promise.resolve().then(function() {
    return self.runCommand(cmd);
  }).then(function(result) {
    ASSERT_EQ(expectedStatus, result.status, result.output);
    var resultLines = result.output.split('\n');
    // Trim the last line if empty to reduce boilerplate.
    if (resultLines[resultLines.length - 1] === '') {
      resultLines = resultLines.slice(0, resultLines.length -1);
    }
    for (var i = 0; i < resultLines.length && i < expectedOutput.length; i++) {
      if (typeof expectedOutput[i] === 'string') {
        EXPECT_EQ(expectedOutput[i], resultLines[i],
            'match on line ' + i);
      } else {
        EXPECT_TRUE(resultLines[i].match(expectedOutput[i]) !== null,
            'match on line ' + i + ' with: ' + resultLines[i]);
      }
    }
    ASSERT_EQ(expectedOutput.length, resultLines.length, 'line count match');
  });
};

DevEnvTest.prototype.initFileSystem = function() {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.postMessage({name: 'file_init'});
    return self.devEnv.wait();
  }).then(function(msg) {
    ASSERT_EQ('file_init_reply', msg.name);
  });
};

DevEnvTest.prototype.writeFile = function(fileName, data) {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.postMessage({
      'name': 'file_write',
      'file': fileName,
      'data': data
    });
    return self.devEnv.wait();
  }).then(function(msg) {
    ASSERT_EQ('file_write_reply', msg.name);
  });
};

DevEnvTest.prototype.setRepo = function(data) {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.postMessage({
      'name': 'set_repo',
      'data': data
    });
    return self.devEnv.wait();
  }).then(function(msg) {
    ASSERT_EQ('set_repo_reply', msg.name);
  });
};

DevEnvTest.prototype.readFile = function(fileName) {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.postMessage({
      'name': 'file_read',
      'file': fileName,
    });
    return self.devEnv.wait();
  }).then(function(msg) {
    ASSERT_EQ('file_read_reply', msg.name);
    return msg.data;
  });
};

DevEnvTest.prototype.mkdir = function(fileName) {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.postMessage({
      'name': 'file_mkdir',
      'file': fileName
    });
    return self.devEnv.wait();
  }).then(function(msg) {
    ASSERT_EQ('file_mkdir_reply', msg.name);
  });
};

DevEnvTest.prototype.rmRf = function(fileName) {
  var self = this;
  return Promise.resolve().then(function() {
    self.devEnv.postMessage({
      'name': 'file_rm_rf',
      'file': fileName
    });
    return self.devEnv.wait();
  }).then(function(msg) {
    ASSERT_EQ('file_rm_rf_reply', msg.name);
  });
};

/**
 * Convert an Array to a string.
 * @param {Array} arr The Array to be converted.
 * @returns {string}
 */
DevEnvTest.array2Str = function(arr) {
  return String.fromCharCode.apply(null, arr);
};

/**
 * Convert a string to an Array.
 * @param {string} str The String to be converted.
 * @returns {Array} Bytes sent.
 */
DevEnvTest.str2Array = function(str) {
  var arr = [];
  for (var i = 0; i < str.length; i++) {
    arr.push(str.charCodeAt(i));
  }
  return arr;
};

/**
 * Execute a TCP command, and listen for the reply. Buffer incoming TCP
 * messages, and resolve when we receive the reply to the command.
 * @private
 * @param {object} msg The message to send.
 * @returns {Promise}
 */
DevEnvTest.prototype.tcpExec_ = function(msg) {
  var self = this;
  var reply = msg.name + '_reply';
  var error = msg.name + '_error';
  return Promise.resolve().then(function() {
    self.tcp.postMessage(msg);
    return self.waitWhile(self.tcp,
      function condition(msg) {
        return msg.name !== reply && msg.name !== error;
      },
      function body(msg) {
        if (msg.name === 'tcp_message') {
          if (self.tcpBuffer[msg.socket] === undefined) {
            self.tcpBuffer[msg.socket] = '';
          }
          self.tcpBuffer[msg.socket] += DevEnvTest.array2Str(msg.data);
        } else if (msg.name === 'tcp_disconnect') {
          self.tcpDisconnected[msg.socket] = true;
        } else {
          // Is there a better way to do this?
          chrometest.assert(false, 'unexpected message ' + msg.name +
              ' from tcpapp');
        }
      });
  }).then(function(msg) {
    ASSERT_EQ(reply, msg.name, 'Error is: ' + msg.error);
    return msg;
  });
};

/**
 * Initiate a TCP connection.
 * @param {string} addr The address of the remote host.
 * @param {number} port The port to connect to.
 * @returns {Promise}
 */
DevEnvTest.prototype.tcpConnect = function(addr, port) {
  return this.tcpExec_({
    name: 'tcp_connect',
    addr: addr,
    port: port
  });
};

/**
 * Send a string over TCP.
 * @param {number} socket The TCP socket.
 * @param {string} msg The string to be sent.
 * @returns {Promise}
 */
DevEnvTest.prototype.tcpSend = function(socket, msg) {
  var self = this;
  return self.tcpExec_({
    name: 'tcp_send',
    socket: socket,
    data: DevEnvTest.str2Array(msg)
  }).then(function(msg) {
    return msg.data;
  });
};

/**
 * Receive a TCP message. Resolves with a String of the received data, or null
 * if the TCP connection has been closed.
 * @param {number} socket The TCP socket.
 * @returns {Promise}
 */
DevEnvTest.prototype.tcpRecv = function(socket) {
  var self = this;
  return new Promise(function(resolve, reject) {
    if (self.tcpBuffer[socket] !== undefined) {
      var buffer = self.tcpBuffer[socket];
      delete self.tcpBuffer[socket];
      resolve(buffer);
    } else if (self.tcpDisconnected[socket]) {
      delete self.tcpDisconnected[socket];
      resolve(null);
    } else {
      self.tcp.wait().then(function(msg) {
        if (msg.name === 'tcp_message') {
          resolve(DevEnvTest.array2Str(msg.data));
        } else if (msg.name === 'tcp_disconnect') {
          resolve(null);
        } else {
          chrometest.assert(false, 'unexpected message ' + msg.name +
              ' from tcpapp');
        }
      });
    }
  });
};

/**
 * Close a TCP connection.
 * @param {number} socket The TCP socket.
 * @returns {Promise}
 */
DevEnvTest.prototype.tcpClose = function(socket) {
  return this.tcpExec_({
    name: 'tcp_close',
    socket: socket
  });
};
