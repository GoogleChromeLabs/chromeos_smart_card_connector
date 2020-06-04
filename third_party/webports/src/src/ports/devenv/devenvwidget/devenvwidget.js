/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This plugin allows clients to include a DevEnvWidget in a web page, which
 * uses GCC in the NaCl Development Environment extension to compile and run
 * the contents of a text input.
 *
 * Example usage:
 * var widget = new DevEnvWidget({
 *   source: document.getElementById('source'), // <textarea>
 *   run: document.getElementById('run'), // <button> or other clickable
 *   status: document.getElementById('status'), // <div> or other output area
 *   output: document.getElementById('output') // <div> or other output area
 * });
 */

(function(global) {
  /*
   * Create a new Widget. This is the constructor called by clients.
   */
  function Widget(args) {
    var self = this;

    this.input = args.source;

    var extensionID = args.extension || Widget.DEFAULT_EXTENSION_ID;
    this.model = new DevEnv(extensionID);
    this.running = false;

    this.view = new WidgetView(args);
    this.view.setRunAction(this.run.bind(this));
    this.view.setStatus(WidgetView.STATUS_IDLE);

    this.model.setStdoutListener(function(output) {
      self.view.appendOutput(output);
    });

    this.ready = false;
    this.model.checkExtension().then(function() {
      self.ready = true;
    }, function(e) {
      self.view.setStatus(WidgetView.STATUS_ERROR);
      self.view.appendHTMLOutput(e.message);
    });
  }

  Widget.WORKING_DIR = '/tmp';
  Widget.C_FILE = Widget.WORKING_DIR + '/widget.c';
  Widget.COMPILE_COMMAND = 'gcc ' + Widget.WORKING_DIR + '/widget.c' +
      ' -o ' + Widget.WORKING_DIR + '/widget -lmingn';
  Widget.RUN_COMMAND = Widget.WORKING_DIR + '/widget';

  // The extension ID of the NaCl Development Environment extension.
  Widget.DEFAULT_EXTENSION_ID = 'aljpgkjeipgnmdpikaajmnepbcfkglfa';

  // Helper function to check that a program executed successfully.
  Widget.checkExitStatus = function(msg, error) {
    return new Promise(function(resolve, reject) {
      if (msg.status !== 0) {
        if (!error)
          error = 'program returned exit code ' + msg.status;
        reject(new Error(error));
        return;
      }
      resolve(msg);
    });
  };

  // Compile and run the contents of the Widget textarea.
  Widget.prototype.run = function() {
    var self = this;
    if (this.running || !this.ready) {
      return;
    }
    this.running = true;
    self.view.setStatus(WidgetView.STATUS_COMPILING);
    self.view.clearOutput();
    self.model.connect();
    self.model.post({
      name: 'file_init'
    }).then(function() {
      return self.model.post({
        name: 'file_remove',
        file: Widget.C_FILE
      }).catch(function() {
        // Ignore the error that will be thrown if the file is not there to be
        // removed.
      });
    }).then(function() {
      return self.model.post({
        name: 'file_write',
        file: Widget.C_FILE,
        data: self.input.value
      });
    }).then(function() {
      return self.model.post(DevEnv.makeSpawn(Widget.COMPILE_COMMAND));
    }).then(function(msg) {
      return self.model.post(DevEnv.makeWaitpid(msg.pid));
    }).then(function(msg) {
      return Widget.checkExitStatus(msg, 'compilation failed');
    }).then(function() {
      self.view.setStatus(WidgetView.STATUS_RUNNING);
      return self.model.post(DevEnv.makeSpawn(Widget.RUN_COMMAND));
    }).then(function(msg) {
      return self.model.post(DevEnv.makeWaitpid(msg.pid));
    }).then(function(msg) {
      return Widget.checkExitStatus(msg);
    }).then(function() {
      self.running = false;
      self.view.setStatus(WidgetView.STATUS_IDLE);
      self.mode.disconnect();
    }, function(e) {
      self.running = false;
      self.view.setStatus(WidgetView.STATUS_ERROR);
      self.view.appendOutput('Error: ' + e.message + '.\n');
    });
  };


  // Communicate with the NaCl DevEnv extension to run GCC and the compiled
  // program.
  function DevEnv(extensionID) {
    this.port = null;
    this.extensionID = extensionID;
    this.stdoutListener = function() { };
  }

  // This is the Chrome Web Store URL of the NaCl Development Environment
  // extension.
  DevEnv.CHROME_STORE_URL = 'https://chrome.google.com/webstore/detail/' +
      'aljpgkjeipgnmdpikaajmnepbcfkglfa';

  // This error is thrown when the extension cannot be found.
  DevEnv.ERROR_NO_EXTENSION = 'Cannot find the NaCl Development Environment' +
      ' extension. <a href="' + DevEnv.CHROME_STORE_URL + '" target="_blank">' +
      'Download it from the Chrome Web Store.</a>';

  // This error is thrown when commands are run before the connection is made
  // to the NaCl DevEnv extension.
  DevEnv.ERROR_NOT_CONNECTED = 'not connected to Chrome extension';

  // Helper function to make a spawn command to the extension.
  DevEnv.makeSpawn = function(cmd) {
    return {
      name: 'nacl_spawn',
      nmf: 'bash.nmf',
      argv: ['bash', '-c', '. /mnt/http/setup-environment && ' + cmd],
      cwd: '/home/user',
      envs: []
    };
  };

  // Helper function to make a waitpid command to the extension.
  DevEnv.makeWaitpid = function(pid) {
    return {
      name: 'nacl_waitpid',
      pid: pid,
      options: 0
    };
  };

  // Check existence of the NaCl DevEnv extension.
  DevEnv.prototype.checkExtension = function() {
    var self = this;
    return new Promise(function(resolve, reject) {
      chrome.runtime.sendMessage(
        self.extensionID,
        {name: 'ping'},
        {},
        function (response) {
          if (!response || response.name !== 'pong') {
            reject(new Error(DevEnv.ERROR_NO_EXTENSION));
          } else {
            resolve();
          }
        }
      );
    });
  };

  // Connect to the NaCl DevEnv extension.
  DevEnv.prototype.connect = function() {
    this.port = chrome.runtime.connect(this.extensionID);
    this.port.onMessage.addListener(this.stdoutListener);
  };

  // Disconnect from the NaCl DevEnv extension.
  DevEnv.prototype.disconnect = function() {
    if (!this.port) {
      throw new Error(DevEnv.ERROR_NOT_CONNECTED);
    }
    this.port.disconnect();
    this.port = null;
  };

  // Set a callback to be called when called programs write to stdout.
  DevEnv.prototype.setStdoutListener = function(listener) {
    var oldListener = this.stdoutListener;
    var newListener = function(response) {
      if (response && response.name === 'nacl_stdout') {
        listener(response.output);
      }
    };
    if (this.port) {
      this.port.onMessage.removeListener(oldListener);
      this.port.onMessage.addListener(newListener);
    }
    this.stdoutListener = newListener;
  };

  // Send a message to the NaCl DevEnv extension.
  DevEnv.prototype.post = function(msg) {
    var self = this;
    return new Promise(function(resolve, reject) {
      if (!msg.name) {
        reject(new Error('name field not set'));
      }
      var replyName = msg.name + '_reply';
      var errorName = msg.name + '_error';
      var handleMessage = function(response) {
        if (!response) {
          return;
        } else if (response.name === replyName) {
          self.port.onMessage.removeListener(handleMessage);
          resolve(response);
        } else if (response.name === errorName) {
          self.port.onMessage.removeListener(handleMessage);
          reject(new Error(response.error));
        }
      }
      self.port.postMessage(msg);
      self.port.onMessage.addListener(handleMessage);
    });
  };

  // WidgetView controls the HTML elements that comprise the Widget.
  function WidgetView(args) {
    this.source = args.source;
    this.run = args.run;
    this.status = args.status;
    this.output = args.output;

    this.runAction = null;
  }

  // This error is thrown when setStatus() is called with an unknown status.
  WidgetView.ERROR_UNKNOWN_STATUS = 'unknown status';

  // Define the states of the widget.
  WidgetView.STATUS_IDLE = 0;
  WidgetView.STATUS_COMPILING = 1;
  WidgetView.STATUS_RUNNING = 2;
  WidgetView.STATUS_ERROR = 3;

  // These define how the interface looks for each status (e.g. red
  // background on error).
  WidgetView.STATUS_CSS_PREFIX = 'dew-status-';
  WidgetView.STATUS_CSS_NOERROR = WidgetView.STATUS_CSS_PREFIX + 'noerror';
  WidgetView.STATUS_CSS_ERROR = WidgetView.STATUS_CSS_PREFIX + 'error';
  WidgetView.STATUS_VIEWS_ = {};
  WidgetView.STATUS_VIEWS_[WidgetView.STATUS_IDLE] = {
    cssClass: WidgetView.STATUS_CSS_NOERROR,
    text: 'Idle',
    runEnabled: true
  };
  WidgetView.STATUS_VIEWS_[WidgetView.STATUS_COMPILING] = {
    cssClass: WidgetView.STATUS_CSS_NOERROR,
    text: 'Compiling',
    runEnabled: false
  };
  WidgetView.STATUS_VIEWS_[WidgetView.STATUS_RUNNING] = {
    cssClass: WidgetView.STATUS_CSS_NOERROR,
    text: 'Running',
    runEnabled: false
  };
  WidgetView.STATUS_VIEWS_[WidgetView.STATUS_ERROR] = {
    cssClass: WidgetView.STATUS_CSS_ERROR,
    text: 'Error',
    runEnabled: true
  };

  // Append a string to the output box.
  WidgetView.prototype.appendOutput = function(output) {
    if (this.output) {
      var node = document.createTextNode(output);
      this.output.appendChild(node);
    }
  };

  // Append an HTML string to the output box. Do not use on unsanitized input.
  WidgetView.prototype.appendHTMLOutput = function(output) {
    if (this.output) {
      var node = document.createElement('span');
      node.innerHTML = output;
      this.output.appendChild(node);
    }
  };

  // Clear the output box.
  WidgetView.prototype.clearOutput = function() {
    if (this.output)
      this.output.innerText = '';
  };

  // Add a listener to be run when the Run button is clicked.
  WidgetView.prototype.setRunAction = function(action) {
    if (this.runAction) {
      this.run.removeEventListener('click', this.runAction);
    }
    this.runAction = action;
    this.run.addEventListener('click', this.runAction);
  };

  // Change the status and update the view accordingly.
  WidgetView.prototype.setStatus = function(status) {
    if (!WidgetView.STATUS_VIEWS_[status]) {
      throw new Error(WidgetView.ERROR_UNKNOWN_STATUS);
    }
    var view = WidgetView.STATUS_VIEWS_[status];
    if (this.run)
      this.run.disabled = !view.runEnabled;
    if (this.status)
      this.status.innerText = view.text;
    if (this.output) {
      this.output.classList.remove(WidgetView.STATUS_CSS_NOERROR);
      this.output.classList.remove(WidgetView.STATUS_CSS_ERROR);
      this.output.classList.add(view.cssClass);
    }
  };

  // Expose Widget to the outside.
  global['DevEnvWidget'] = Widget;
})(window);
