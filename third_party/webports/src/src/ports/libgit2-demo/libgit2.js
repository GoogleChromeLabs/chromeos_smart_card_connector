/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';
function runGitCmd(message, arg1, arg2) {
  var dir = document.getElementById('directory').value;
  if (arg2 && arg1)
    embed.postMessage([message, dir, arg1, arg2]);
  else if (arg1)
    embed.postMessage([message, dir, arg1]);
  else
    embed.postMessage([message, dir]);
}

function clone() {
  var url = document.getElementById('clone_url').value;
  runGitCmd('clone', url);
}

function chdir() {
  runGitCmd('chdir');
}

function init() {
  runGitCmd('init');
}

function status() {
  runGitCmd('status');
}

function mount() {
  chrome.fileSystem.chooseEntry({type:"openDirectory"}, function(entry) {
    runGitCmd('mount', entry.filesystem, entry.fullPath);
  });
}

function push() {
  var refspec = document.getElementById('push_refspec').value;
  runGitCmd('push', refspec);
}

function commit() {
  var filename = document.getElementById('commit_filename').value;
  runGitCmd('commit', filename);
}

function handleMessage(message) {
  var output = document.getElementById('output');
  output.value += message.data;
  output.scrollTop = output.scrollHeight;
}

function onLoad() {
  window.embed = document.getElementById('nacl_module');
  embed.addEventListener('message', handleMessage, true);
  document.getElementById('init').addEventListener('click', init);
  document.getElementById('clone').addEventListener('click', clone);
  document.getElementById('status').addEventListener('click', status);
  document.getElementById('mount').addEventListener('click', mount);
  document.getElementById('push').addEventListener('click', push);
  document.getElementById('commit').addEventListener('click', commit);
}

window.onload = onLoad;
