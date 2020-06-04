/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

lib.rtdep('lib.f',
          'hterm',
          'NaClProcessManager');

// CSP means that we can't kick off the initialization from the html file,
// so we do it like this instead.
window.onload = function() {
  lib.init(function() {
    NaClTerm.init();
  });
};

/**
 * This class uses the NaClProcessManager to run NaCl executables within an
 * hterm.
 *
 * @param {Object} argv The argument object passed in from the Terminal.
 */
function NaClTerm(argv) {
  this.argv = argv;
  this.io = argv.io.push();
  this.width = this.io.terminal_.screenSize.width;

  this.bufferedOutput = '';

  // Have we started spawning the initial process?
  this.started = false;

  // Has the initial process finished loading?
  this.loaded = false;

  this.print = this.io.print.bind(this.io);

  var mgr = this.processManager = new NaClProcessManager();
  mgr.setStdoutListener(this.handleStdout_.bind(this));
  mgr.setErrorListener(this.handleError_.bind(this));
  mgr.setRootProgressListener(this.handleRootProgress_.bind(this));
  mgr.setRootLoadListener(this.handleRootLoad_.bind(this));
};

/**
 * Flag for cyan coloring in the terminal.
 * @const
 */
NaClTerm.ANSI_CYAN = '\x1b[36m';

/**
 * Flag for color reset in the terminal.
 * @const
 */
NaClTerm.ANSI_RESET = '\x1b[0m';

/*
 * Character code for Control+C in the terminal.
 * @type {number}
 */
NaClTerm.CONTROL_C = 3;

/**
 * Add the appropriate hooks to HTerm to start the session.
 */
NaClTerm.prototype.run = function() {
  this.io.onVTKeystroke = this.onVTKeystroke_.bind(this);
  this.io.sendString = this.onVTKeystroke_.bind(this);
  this.io.onTerminalResize = this.onTerminalResize_.bind(this);

  this.print(NaClTerm.ANSI_CYAN);
}

/**
 * Static initializer called from index.html.
 *
 * This constructs a new Terminal instance and instructs it to run a NaClTerm.
 */
NaClTerm.init = function() {
  var profileName = lib.f.parseQuery(document.location.search)['profile'];
  var terminal = new hterm.Terminal(profileName);
  terminal.decorate(document.querySelector('#terminal'));

  // Useful for console debugging.
  window.term_ = terminal;

  // We don't properly support the hterm bell sound, so we need to disable it.
  terminal.prefs_.definePreference('audible-bell-sound', '');

  // TODO(bradnelson/rginda): Drop when hterm auto-detects this.
  // Turn on open web friendly clipboard handling if we're not running
  // in a chrome app.
  if (!window.chrome || !chrome.runtime || !chrome.runtime.id) {
    terminal.prefs_.definePreference('use-default-window-copy', true);
    terminal.prefs_.definePreference('ctrl-c-copy', true);
    terminal.prefs_.definePreference('ctrl-v-paste', true);
  }

  terminal.setAutoCarriageReturn(true);
  terminal.setCursorPosition(0, 0);
  terminal.setCursorVisible(true);
  terminal.runCommandClass(NaClTerm, document.location.hash.substr(1));

  return true;
};

/**
 * Handle stdout event from NaClProcessManager.
 * @private
 * @param {string} msg The string sent to stdout.
 */
NaClTerm.prototype.handleStdout_ = function(msg) {
  if (!this.loaded) {
    this.bufferedOutput += msg;
  } else {
    this.print(msg);
  }
}

/**
 * Handle error event from NaCl.
 * @private
 * @param {string} cmd The name of the process with the error.
 * @param {string} err The error message.
 */
NaClTerm.prototype.handleError_ = function(cmd, err) {
  this.print(cmd + ': ' + err + '\n');
}

/**
 * Notify the user when we are done loading a URL.
 * @private
 */
NaClTerm.prototype.doneLoadingUrl_ = function() {
  var width = this.width;
  this.print('\r' + Array(width+1).join(' '));
  var message = '\rLoaded ' + this.lastUrl;
  if (this.lastTotal) {
    var kbsize = Math.round(this.lastTotal/1024)
    message += ' ['+ kbsize + ' KiB]';
  }
  this.print(message.slice(0, width) + '\n')
}

/**
 * Handle load progress event from NaCl for the root process.
 * @private
 * @param {string} url The URL that is being loaded.
 * @param {boolean} lengthComputable Is our progress quantitatively measurable?
 * @param {number} loaded The number of bytes that have been loaded.
 * @param {number} total The total number of bytes to be loaded.
 */
NaClTerm.prototype.handleRootProgress_ = function(
      url, lengthComputable, loaded, total) {
  if (url !== undefined)
    url = url.substring(url.lastIndexOf('/') + 1);

  if (this.lastUrl && this.lastUrl !== url)
    this.doneLoadingUrl_()

  if (!url)
    return;

  this.lastUrl = url;
  this.lastTotal = total;

  var message = 'Loading ' + url;
  if (lengthComputable && total) {
    var percent = Math.round(loaded * 100 / total);
    var kbloaded = Math.round(loaded / 1024);
    var kbtotal = Math.round(total / 1024);
    message += ' [' + kbloaded + ' KiB/' + kbtotal + ' KiB ' + percent + '%]';
  }

  this.print('\r' + message.slice(-this.width));
}

/**
 * Handle load end event from NaCl for the root process.
 * @private
 */
NaClTerm.prototype.handleRootLoad_ = function() {
  if (this.lastUrl)
    this.doneLoadingUrl_();
  else
    this.print('Loaded.\n');

  this.print(NaClTerm.ANSI_RESET);

  // Now that have completed loading and displaying
  // loading messages we output any messages from the
  // NaCl module that were buffered up unto this point
  this.loaded = true;
  this.print(this.bufferedOutput);
  this.bufferedOutput = '';
}

/**
 * Clean up once the root process exits.
 * @private
 * @param {number} pid The PID of the process that exited.
 * @param {number} status The exit code of the process.
 */
NaClTerm.prototype.handleExit_ = function(pid, status) {
  this.print(NaClTerm.ANSI_CYAN)

  // The root process finished.
  if (status === -1) {
    this.print('Program (' + NaClTerm.nmf +
                         ') crashed (exit status -1)\n');
  } else {
    this.print('Program (' + NaClTerm.nmf + ') exited ' +
               '(status=' + status + ')\n');
  }
  this.argv.io.pop();
  // Remove window close handler.
  window.onbeforeunload = function() { return null; };
  if (this.argv.onExit) {
    this.argv.onExit(status);
  }
}

/**
 * Spawn the root process (usually bash).
 * @private
 *
 * We delay this call until the first terminal resize event so that we start
 * with the correct size.
 */
NaClTerm.prototype.spawnRootProcess_ = function() {
  var self = this;
  var argv = NaClTerm.argv || [];
  var env = NaClTerm.env || [];
  var cwd = NaClTerm.cwd || '/';
  argv = [NaClTerm.nmf].concat(argv);

  try {
    var handleSuccess = function(naclType) {
      self.print('Loading NaCl module.\n');
      self.processManager.spawn(
          NaClTerm.nmf, argv, env, cwd, naclType, null, function(rootPid) {
        // Warn if we close while still running.
        window.onbeforeunload = function() {
          return 'Processes still running!';
        };
        self.processManager.waitpid(rootPid, 0, self.handleExit_.bind(self));
      });
    };
    var handleFailure = function(message) {
      self.print(message);
    };
    self.processManager.checkUrlNaClManifestType(
        NaClTerm.nmf, handleSuccess, handleFailure);
  } catch (e) {
    self.print(e.message);
  }

  self.started = true;
}

/**
 * Handle hterm terminal resize events.
 * @private
 * @param {number} width The width of the terminal.
 * @param {number} height The height of the terminal.
 */
NaClTerm.prototype.onTerminalResize_ = function(width, height) {
  this.processManager.onTerminalResize(width, height);
  if (!this.started) {
    this.spawnRootProcess_();
  }
}

/**
 * Handle hterm keystroke events.
 * @private
 * @param {string} str The characters sent by hterm.
 */
NaClTerm.prototype.onVTKeystroke_ = function(str) {
  try {
    if (str.charCodeAt(0) === NaClTerm.CONTROL_C) {
      if (this.processManager.sigint()) {
        this.print('\n');
        return;
      }
    }
    this.processManager.sendStdinForeground(str);
  } catch (e) {
    this.print(e.message);
  }
}
