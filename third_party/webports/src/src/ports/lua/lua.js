/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

NaClTerm.nmf = 'lua.nmf'
NaClTerm.env = ['NACL_DATA_MOUNT_FLAGS=manifest=/manifest.txt']

function log(message) {
  document.getElementById('log').textContent = message;
}

function fsErrorHandler(error) {
  log("Filesystem error: "+ error);
}

function uploadFile(file) {
  fs.root.getFile(file.name, {create: true, exclusive: true},
  function(fileEntry) {
    fileEntry.createWriter(function(fileWriter) {
    // Note: write() can take a File or Blob object.
    fileWriter.write(file);
    log("File uploaded!\n");
    }, fsErrorHandler);
  }, fsErrorHandler);
}

function uploadFiles(evt) {
  var files = this.files;
  for (var i = 0, file; file = files[i]; ++i) {
    uploadFile(file)
  }
}

function onInitFS(fs) {
  var upload = document.getElementById('upload');
  if (upload !== null) {
    upload.addEventListener('change', uploadFiles, false);
    window.fs = fs
  }
  NaClTerm.init();
}

function onInit() {
  navigator.webkitPersistentStorage.requestQuota(1024 * 1024,
    function(bytes) {
      window.webkitRequestFileSystem(window.PERSISTENT, bytes, onInitFS)
    },
    function() {
      log("Failed to allocate space!\n");
      // Start the terminal even if FS failed to init.
      NaClTerm.init();
    }
  );
}

window.onload = function() {
  lib.init(function() {
    onInit();
  });
};
