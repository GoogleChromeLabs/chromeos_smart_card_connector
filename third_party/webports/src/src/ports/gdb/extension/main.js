/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// The devtools page for the debugger.

lib.rtdep('lib.f',
          'hterm');


/**
 * Keep a global reference to the message port used to talk to the background
 * page.
 */
var g_backgroundPort;


/**
 * A Gdb Terminal.
 * @constructor.
 */
function GdbTerm(argv) {
  this.io_ = argv.io.push();
  window.io_ = this.io_;
  this.argv_ = argv;
}

/**
 * Handle a terminal resize.
 * @param {integer} width Width of the terminal in characters.
 * @param {integer} height Height of the terminal in characters.
 */
GdbTerm.prototype.onTerminalResize_ = function(width, height) {
  g_backgroundPort.postMessage({
    'name': 'input',
    'msg': {'tty_resize': [ width, height ]},
  });
  var moduleList = document.getElementById('modules');
  if (moduleList.hasChildNodes() && moduleList.selectedIndex < 0) {
    moduleList.firstChild.selected = true;
    updateSelection();
  }
}

/**
 * Handle a keystroke from the user.
 * @param {string} str The keystroke.
 */
GdbTerm.prototype.onVTKeystroke_ = function(str) {
  g_backgroundPort.postMessage({
    'name': 'input',
    'msg': {'gdb': str},
  });
}

/**
 * Setup handlers after terminal start.
 */
GdbTerm.prototype.run = function() {
  this.io_.onVTKeystroke = this.onVTKeystroke_.bind(this);
  this.io_.onTerminalResize = this.onTerminalResize_.bind(this);
}


/**
 * Resize the config area to a given size.
 * @param {integer} sz The width in pixels.
 */
function setWidth(sz) {
  var sized = document.getElementById('sized');
  var terminal = document.getElementById('terminal');
  var sizer = document.getElementById('sizer');
  var rowWidth = document.body.clientWidth;
  sz = Math.max(Math.min(sz, rowWidth - 100), 200);
  var newTerminalWidth = rowWidth - sz - sizer.clientWidth - 1;
  sized.style.width = sz + 'px';
  terminal.style.width = newTerminalWidth + 'px';
}

/**
 * Fixup the config area in the event of a reflow.
 */
function fixupWidth() {
  var sized = document.getElementById('sized');
  setWidth(sized.clientWidth);
}

/**
 * Enable resizing by installing event handlers.
 */
function enableSizer() {
  var sizer = document.getElementById('sizer');
  var sized = document.getElementById('sized');
  var terminal = document.getElementById('terminal');
  var baseX;
  var startWidth;

  function move(e) {
    setWidth(startWidth + e.clientX - baseX);
  }

  function up(e) {
    window.removeEventListener('mousemove', move, true);
    window.removeEventListener('mouseup', up, true);
    terminal.style.pointerEvents = 'auto';
  }

  sizer.addEventListener('mousedown', function(e) {
    e.preventDefault();
    baseX = e.clientX;
    startWidth = sized.clientWidth;
    document.body.focus();
    window.addEventListener('mousemove', move, true);
    window.addEventListener('mouseup', up, true);
    terminal.style.pointerEvents = 'none';
  }, true);
}

/**
 * Handle load events by creating an hterm.
 */
window.onload = function() {
  lib.init(function() {
    var profileName = lib.f.parseQuery(document.location.search)['profile'];
    var terminal = new hterm.Terminal(profileName);
    terminal.decorate(document.querySelector('#terminal'));

    // Useful for console debugging.
    window.term_ = terminal;

    // We don't properly support the hterm bell sound, so we need to disable it.
    terminal.prefs_.definePreference('audible-bell-sound', '');

    terminal.setAutoCarriageReturn(true);
    terminal.setCursorPosition(0, 0);
    terminal.setCursorVisible(true);
    terminal.prefs_.set('foreground-color', 'black');
    terminal.prefs_.set('background-color', 'white');
    terminal.runCommandClass(GdbTerm, document.location.hash.substr(1));
  });
  enableSizer();
  fixupWidth();
  setupBackgroundPort();
};

/**
 * Handle window resizes by ensuring the config terminal split stays self
 * consistent.
 */
window.addEventListener('resize', function(e) {
  fixupWidth();
});

/**
 * Check if a module title indicates it's GDB.
 * @param {string} title The title string of a module.
 * @return {boolean} True if it's GDB.
 */
function isGdb(title) {
  return title.match(/^Native Client module:.*gdb.nmf$/);
}

/**
 * Given a module, return its title.
 * @param {Object} module An object containing info on one module.
 * @return {string} The title of the module.
 */
function taskTitle(module) {
  var title = module.process.title;
  var m = title.match(/^Native Client module: (.*)$/);
  if (isGdb(title)) {
    title = '[GDB]';
  } else if (!m || m.length !== 2) {
    title = module.title;
  } else {
    title = m[1];
  }
  title += ' (' + module.debugState + ')';
  return title;
}

/**
 * Given a module, return a detailed string about it.
 * @param {Object} module An object containing info on one module.
 * @return {string} A string describing details about the module.
 */
function taskInfo(module) {
  return 'id: ' + module.process.id + ', pid: ' + module.process.osProcessId +
         ', port: ' + module.process.naclDebugPort;
}

/**
 * Attempt to attach to the current module.
 */
function attach(e) {
  // Halt events here to prevent a bad interaction with hterm.
  e.stopPropagation();
  e.preventDefault();
  var modules = document.getElementById('modules');
  g_backgroundPort.postMessage({
    'name': 'attach',
    'processId': modules.options[modules.selectedIndex].id,
  });
}

/**
 * Attempt to kill the current module.
 */
function kill(e) {
  // Halt events here to prevent a bad interaction with hterm.
  e.stopPropagation();
  e.preventDefault();
  var modules = document.getElementById('modules');
  g_backgroundPort.postMessage({
    'name': 'kill',
    'processId': modules.options[modules.selectedIndex].id,
  });
}

/**
 * Attempt manual setup of GDB.
 */
function attachManual(e) {
  // Halt events here to prevent a bad interaction with hterm.
  e.stopPropagation();
  e.preventDefault();
  g_backgroundPort.postMessage({
    'name': 'attachManual',
  });
}

/**
 * Broadcast a change in configuration.
 */
function configChanged() {
  g_backgroundPort.postMessage({
    'name': 'settingsChange',
    'settings': {
      'onStartRun':  document.getElementById('onStartRun').checked,
      'onFaultAttach': document.getElementById('onFaultAttach').checked,
      'showGdb': document.getElementById('showGdb').checked,
    },
  });
}

/**
 * React to a change in the selected module.
 */
function updateSelection() {
  var modules = document.getElementById('modules');
  var moduleInfo = document.getElementById('moduleInfo');
  // Clear the terminal screen.
  window.io_.print('\x1b[H\x1b[2J');
  if (modules.selectedIndex >= 0) {
    var pick = modules.options[modules.selectedIndex];
    moduleInfo.innerText = pick.title;
    g_backgroundPort.postMessage({name: 'tune', 'processId': pick.id});
    // Send Ctrl-L
    g_backgroundPort.postMessage({
      'name': 'input',
      'msg': {'gdb': '\x0c'},
    });
  } else {
    moduleInfo.innerText = '';
    g_backgroundPort.postMessage({name: 'tune', 'processId': null});
  }
}

/**
 * Register a bunch of ui handlers on load.
 */
window.addEventListener('load', function() {
  document.getElementById('onStartRun').addEventListener(
      'click', configChanged);
  document.getElementById('onStartAttach').addEventListener(
      'click', configChanged);
  document.getElementById('onFaultAttach').addEventListener(
      'click', configChanged);
  document.getElementById('onFaultTerminate').addEventListener(
      'click', configChanged);
  document.getElementById('showGdb').addEventListener(
      'click', configChanged);
  var attachElement = document.getElementById('attach');
  var killElement = document.getElementById('kill');
  var modules = document.getElementById('modules');
  attachElement.disabled = true;
  killElement.disabled = true;
  attachElement.addEventListener('click', attach);
  killElement.addEventListener('click', kill);
  modules.addEventListener('click', function() {
    attachElement.disabled = false;
    killElement.disabled = false;
    updateSelection();
  });
  modules.addEventListener('dblclick', attach);
  document.getElementById('manual').addEventListener('click', attachManual);
});

/**
 * Update the UI with a change in settings or modules from elsewhere.
 * @param {Object} msg A dict capturing debugger state.
 */
function handleChange(msg) {
  var settings = msg.settings;
  var modules = msg.naclModules;

  var moduleList = document.getElementById('modules');
  document.getElementById('onStartRun').checked = settings.onStartRun;
  document.getElementById('onStartAttach').checked = !settings.onStartRun;
  document.getElementById('onFaultAttach').checked = settings.onFaultAttach;
  document.getElementById('onFaultTerminate').checked =
      !settings.onFaultAttach;
  document.getElementById('showGdb').checked = settings.showGdb;

  function Visible(processId) {
    if (!(processId in modules)) {
      return false;
    }
    return msg.settings.showGdb || !isGdb(modules[processId].process.title);
  }

  for (var i = 0; i < moduleList.childNodes.length; i++) {
    var child = moduleList.childNodes[i];
    if (!Visible(child.id)) {
      moduleList.removeChild(child);
      --i;
    }
  }
  for (var processId in modules) {
    var module = modules[processId];
    if (!Visible(processId))
      continue;
    var item = document.getElementById(processId);
    if (item === null) {
      item = document.createElement('option');
      item.setAttribute('id', processId);
      moduleList.appendChild(item);
    } else {
      item.removeChild(item.firstChild);
    }
    item.appendChild(document.createTextNode(taskTitle(module)));
    item.setAttribute('title', taskInfo(module));
  }
}

/**
 * Setup communication with the background page.
 */
function setupBackgroundPort() {
  g_backgroundPort = chrome.runtime.connect({name: 'join'});

  g_backgroundPort.onDisconnect.addListener(function(msg) {
    // reconnect.
    g_backgroundPort = chrome.runtime.connect({name: 'join'});
  });

  g_backgroundPort.onMessage.addListener(function(msg) {
    if (msg.name === 'change') {
      handleChange(msg);
    } else if (msg.name === 'message') {
      if (msg.data.substr(0, 3) === 'gdb') {
        window.io_.print(msg.data.substr(3));
      }
    }
  });
}
