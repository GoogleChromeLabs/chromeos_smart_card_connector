/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

NaClTerm.nmf = 'vim.nmf'

function log(message) {
  console.log(message)
}

function fsErrorHandler(error) {
  // TODO(sbc): Add popup that tells the user there was an error.
  log('Filesystem error: ' + error);
}

function runVim() {
  NaClTerm.init();
}

function runVimWithFile(file) {
  log('runVimWithFile: ' + file.name);
  tempFS.root.getFile(file.name, {create: true},
    function(fileEntry) {
      window.tmpFileEntry = fileEntry
      fileEntry.createWriter(function(fileWriter) {
        // Note: write() can take a File or Blob object.
        fileWriter.write(file);
        log('File written to temporary filesystem\n');
        NaClTerm.argv = ['/tmp/' + file.name];
        runVim();
      }, fsErrorHandler);
    }, fsErrorHandler);
}

function runVimWithFileEntry(fileEntry) {
  window.fileEntryToSave = fileEntry;
  fileEntry.file(function(file) {
    runVimWithFile(file);
  });
}

function uploadDidChange(event) {
  var file = event.target.files[0];
  runVimWithFile(file);
}

function onInitFS(fs) {
  log('onInitFS');
  window.tempFS = fs

  // Once the temp filesystem is initialised we launch vim.
  // For packaged apps the fileEntryToLoad attribute will be set if the
  // user launched us with a fileEntry.  Otherwise we fallback to asking
  // them using chooseEntry.
  if (window.fileEntryToLoad !== undefined) {
    // We have fileEntry already.
    runVimWithFileEntry(window.fileEntryToLoad);
  } else if (chrome.fileSystem) {
    // We have access the fileSystem API, so ask the user
    // to select a file.
    chrome.fileSystem.chooseEntry(function(fileEntry) {
      if (fileEntry) {
        runVimWithFileEntry(fileEntry);
      } else {
        runVim();
      }
    });
  } else {
    // Fall back to using html file selection.
    var upload = document.getElementById('infile');
    if (upload !== null) {
      upload.addEventListener('change', uploadDidChange, false);
    } else {
      runVim();
    }
  }
}

function onInit() {
  navigator.webkitPersistentStorage.requestQuota(1024 * 1024,
    function(bytes) {
      window.webkitRequestFileSystem(window.TEMPORARAY, bytes, onInitFS)
    },
    function() {
      log('Failed to allocate space!\n');
      // Start the terminal even if FS failed to init.
      runVim();
    }
  );
}

window.onload = function() {
  lib.init(function() {
    onInit();
  });
};
