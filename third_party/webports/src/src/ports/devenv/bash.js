/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

function addMount(mountPoint, entry, localPath, mounted) {
  g_mount.mountPoint = mountPoint;
  g_mount.entry = entry;
  g_mount.localPath = localPath;
  g_mount.mounted = mounted;
  g_mount.entryId = '';
}

function handleChooseFolder(mount, callback) {
  chrome.fileSystem.chooseEntry({'type': 'openDirectory'}, function(entry) {
    chrome.fileSystem.getDisplayPath(entry, function(path) {
      mount.entry = entry;
      mount.filesystem = entry.filesystem;
      mount.fullPath = entry.fullPath;
      mount.entryId = chrome.fileSystem.retainEntry(entry);
      mount.localPath = path;
      chrome.storage.local.set({oldMounts: [mount.entryId]}, callback);
    });
  });
}

function restoreMount(mount, callback) {
  // Skip restore mount if running in a page, so we don't get stuck.
  if (chrome.storage === undefined) {
    callback();
    return;
  }
  chrome.storage.local.get('oldMounts', function(items) {
    var oldMounts = items['oldMounts'];
    if (oldMounts === undefined || oldMounts.length === 0) {
      callback();
      return;
    }
    // TODO(gdeepti): Support multiple mounts.
    chrome.fileSystem.restoreEntry(oldMounts[0], function(entry) {
      chrome.fileSystem.getDisplayPath(entry, function(path) {
        mount.entry = entry;
        mount.filesystem = entry.filesystem;
        mount.fullPath = entry.fullPath;
        mount.entryId = chrome.fileSystem.retainEntry(entry);
        mount.localPath = path;
        callback();
      });
    });
  });
}

function handleMount(mount, callback) {
  mount.operationId = 'mount';
  mount.available = true;
  mount.mounted = false;
  var message = {};
  message.mount = mount;
  window.term_.command.processManager.broadcastMessage(message, callback);
}

function handleUnmount(mount, callback) {
  mount.operationId = 'unmount'
  mount.available = false;
  var message = {};
  message.unmount = mount;
  window.term_.command.processManager.broadcastMessage(message, callback);
  addMount('/mnt/local/', null, '', false);
  chrome.storage.local.remove('oldMounts', function() {});
}

function initMountSystem() {
  var terminal = document.getElementById('terminal');
  var mounterClient = new initMounterclient(g_mount, handleChooseFolder,
      handleMount, handleUnmount, terminal);
  addMount('/mnt/local/', null, '', false);
  restoreMount(g_mount, function() {
    initMounter(false, mounterClient, function(update) {
      // Mount any restore mount.
      if (g_mount.entryId !== '') {
        handleMount(g_mount, update);
      }
    });
  });
}

NaClTerm.nmf = 'bash.nmf';
NaClTerm.argv = ['--init-file', '/mnt/http/bashrc'];
NaClTerm.env = ['NACL_DATA_MOUNT_FLAGS=manifest=/manifest.txt']
// Uncomment this line to use only locally built packages
//NaClTerm.env.push('NACL_DEVENV_LOCAL=1')
// Uncomment this line to enable pepper_simple logging
//NaClTerm.env.push('PS_VERBOSITY=3')

function allocateStorage() {
  // Request 10GB storage.
  return new Promise(function(resolve, reject) {
    navigator.webkitPersistentStorage.requestQuota(
        10 * 1024 * 1024 * 1024,
        function(grantedBytes) {
          resolve();
        },
        function() {
          console.log("Failed to allocate space!\n");
          reject();
        }
    );
  });
}

window.onload = function() {
  mounterBackground = document.createElement('div');
  mounterBackground.id = 'mounterBackground';
  mounter = document.createElement('div');
  mounter.id = 'mounter';
  mounterHeader = document.createElement('div');
  mounterHeader.id = 'mounterHeader';
  var text = document.createTextNode('Folders Mounted');
  mounterHeader.appendChild(text);
  mounter.appendChild(mounterHeader);
  mounterBackground.appendChild(mounter);
  var mounterSpacer = document.createElement('div');
  mounterSpacer.id = 'mounterSpacer';

  mounterBackground.appendChild(mounterSpacer);
  mounterThumb = document.createElement('div');
  mounterThumb.id = 'mounterThumb';
  var sp = document.createElement("span");
  var spText = document.createTextNode('. . .');
  sp.appendChild(spText);
  mounterThumb.appendChild(sp);
  mounterBackground.appendChild(mounterThumb);
  document.body.appendChild(mounterBackground);

  lib.init(function() {
    allocateStorage()
        .then(initMountSystem)
        .then(makeRootDir)
        .then(NaClTerm.init);
  });
};

// Patch hterm to intercept Ctrl-Shift-N to create new windows.
hterm.Keyboard.KeyMap.prototype.onCtrlN_ = function(e, keyDef) {
  if (e.shiftKey) {
    chrome.runtime.sendMessage({'name': 'new_window'});
  }
  return '\x0e';
};
