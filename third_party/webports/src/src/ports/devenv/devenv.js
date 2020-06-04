/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

NaClProcessManager.fsroot = '/devenv';

// TODO(sbc): This should be handled by nacl_io or perhaps by NaClProcessManager
function makeRootDir() {
  return new Promise(function(accept, reject) {
    function createDir(rootDirEntry, dirname) {
      rootDirEntry.getDirectory(dirname, {create: true},
          function(dirEntry) {
            accept();
          },
          function() {
            console.log('rootDirEntry.getDirectory failed');
            reject();
          }
      );
    }

    window.requestFileSystem  = window.requestFileSystem
        || window.webkitRequestFileSystem;
    window.requestFileSystem(
        PERSISTENT,
        0,
        function(fs) { createDir(fs.root, NaClProcessManager.fsroot); },
        function() {
          console.log('requestFileSystem failed!');
          reject();
        }
    );
  });
}
