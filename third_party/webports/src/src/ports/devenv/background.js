/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

function newWindow() {
  chrome.app.window.create('bash.html', {
    'bounds': {
      'width': 800,
      'height': 600,
    },
  });
}

chrome.app.runtime.onLaunched.addListener(function() {
  newWindow();
});

chrome.runtime.onMessage.addListener(
  function(request, sender, sendResponse) {
    if (request.name === 'new_window') {
      newWindow();
    }
  }
);

chrome.runtime.onMessageExternal.addListener(
  function(request, sender, sendResponse) {
    if (request.name === 'ping') {
      sendResponse({name: 'pong'});
    }
  }
);

chrome.runtime.onConnectExternal.addListener(function(port) {
  var files = new FileManager();
  var manager = new NaClProcessManager();
  // Assume a default terminal size for headless processes.
  manager.onTerminalResize(80, 24);
  manager.setStdoutListener(function(output) {
    port.postMessage({name: 'nacl_stdout', output: output});
  });

  function fileReply(name, data) {
    return function() {
      port.postMessage({name: name, data: data});
    };
  }
  function fileError(name) {
    return function(error) {
      port.postMessage({name: name, error: error.message});
    };
  }

  port.onMessage.addListener(function(msg) {
    switch (msg.name) {
      case 'nacl_spawn':
        var handleSuccess = function(naclType) {
          manager.spawn(
              msg.nmf, msg.argv, msg.envs, msg.cwd, naclType, null,
              function(pid) {
            port.postMessage({name: 'nacl_spawn_reply', pid: pid});
          });
        };
        var handleFailure = function(message) {
          port.postMessage({name: 'nacl_spawn_reply', pid: -1,
                            message: message});
        };
        if ('naclType' in msg) {
          handleSuccess(msg.naclType);
        } else {
          manager.checkUrlNaClManifestType(
              msg.nmf, handleSuccess, handleFailure);
        }
        break;
      case 'nacl_waitpid':
        manager.waitpid(msg.pid, msg.options, function(pid, status) {
          port.postMessage({
            name: 'nacl_waitpid_reply',
            pid: pid,
            status: status
          });
        });
        break;
      case 'nacl_sigint':
        manager.sigint();
        break;

      case 'set_repo':
        /*
         * Copy the the local NaCl.conf into /usr/etc/pkg/repos/ replacing
         * the origin.
         */
        console.log('set_repo')
        getNaClArch(function(arch) {
          var xhr = new XMLHttpRequest();
          xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
              if (xhr.status != 200) {
                fileError('set_repo_error status=' + xhr.status)('xhr error');
                return;
              }
              var data = xhr.responseText.replace('http://localhost:5103',
                  msg.data);
              files.writeText('/usr/etc/pkg/repos/NaCl.conf', data).then(
                  fileReply('set_repo_reply'), fileError('set_repo_error'));
            }
          };
          xhr.open('GET', '/repos_local_' + arch + '/NaCl.conf', true);
          xhr.send();
        });
        break;

      case 'file_init':
        files.init().then(
            fileReply('file_init_reply'), fileError('file_init_error'));
        break;
      case 'file_read':
        files.readText(msg.file).then(function(data) {
          port.postMessage({
            name: 'file_read_reply',
            data: data
          });
        }, fileError('file_read_error'));
        break;
      case 'file_write':
        files.writeText(msg.file, msg.data).then(
            fileReply('file_write_reply'), fileError('file_write_error'));
        break;
      case 'file_remove':
        files.remove(msg.file).then(
            fileReply('file_remove_reply'), fileError('file_remove_error'));
        break;
      case 'file_mkdir':
        files.makeDirectory(msg.file).then(
            fileReply('file_mkdir_reply'), fileError('file_mkdir_error'));
        break;
      case 'file_rm_rf':
        files.removeDirectory(msg.file).then(
            fileReply('file_rm_rf_reply'), fileError('file_rm_rf_error'));
        break;

      default:
        port.postMessage({name: 'unknown_error', error: 'unknown message'});
    }
  });
});

/**
 * Access the HTML5 Filesystem.
 */
function FileManager() {
  this.persistentFs = null;
  this.temporaryFs = null;
  this.isInitialized = false;
}

/**
 * The size of the requested HTML5 file system, in bytes.
 * @type {number}
 */
FileManager.FS_SIZE = 1024*1024;

/**
 * The path to the tmp directory.
 * @const
 */
FileManager.TMP_MOUNT_POINT = '/tmp';

/**
 * The error thrown when the user tries to perform file operations before the
 * file system has been initialized.
 * @const
 */
FileManager.FS_NOT_INITIALIZED = 'File system not initialized.';

/**
 * Translate a path from the point of view of NaCl to an HTML5 file system and
 * a path within that file system.
 * @param {string} path The path to be translated.
 * @returns {Object} pathInfo An object containing the filesystem and path.
 * @returns {FileSystem} pathInfo.fs The HTML5 filesystem.
 * @returns {string} pathInfo.path The translated path.
 */
FileManager.prototype.translatePath = function(path) {
  if (path.indexOf(FileManager.TMP_MOUNT_POINT) === 0) {
    return {
      fs: this.temporaryFs,
      path: path.replace(FileManager.TMP_MOUNT_POINT, '')
    };
  } else {
    return {
      fs: this.persistentFs,
      path: NaClProcessManager.fsroot + path
    };
  }
};

/**
 * Initialize the file systems. This must be called before any file operations
 * can be performed.
 */
FileManager.prototype.init = function() {
  var self = this;

  function requestFs(type) {
    return new Promise(function(resolve, reject) {
      window.webkitRequestFileSystem(type, FileManager.FS_SIZE,
                                     resolve, reject);
    });
  }

  return Promise.all([
    requestFs(window.PERSISTENT),
    requestFs(window.TEMPORARY),
  ]).then(function(filesystems) {
    self.persistentFs = filesystems[0];
    self.temporaryFs = filesystems[1];
    self.isInitialized = true;
  }).then(makeRootDir);
};

/**
 * Read the contents of a text file.
 * @param {string} fileName The name of the file to be read.
 */
FileManager.prototype.readText = function(fileName) {
  var self = this;
  return new Promise(function(resolve, reject) {
    if (!self.isInitialized) {
      reject(new Error(FileManager.FS_NOT_INITIALIZED));
      return;
    }

    var pathInfo = self.translatePath(fileName);
    pathInfo.fs.root.getFile(pathInfo.path, {}, function(fe) {
      fe.file(function(file) {
        var reader = new FileReader();
        reader.onload = function(e) {
          resolve(this.result);
        };
        reader.onerror = function(e) {
          reject(new Error(e.message));
        };
        reader.readAsText(file);
      }, reject);
    }, reject);
  });
};

/**
 * Write a string to a text file.
 * @param {string} fileName The name of the destination file.
 * @param {string} fileName The text to be written.
 */
FileManager.prototype.writeText = function(fileName, content) {
  var self = this;
  return new Promise(function(resolve, reject) {
    if (!self.isInitialized) {
      reject(new Error(FileManager.FS_NOT_INITIALIZED));
      return;
    }

    var pathInfo = self.translatePath(fileName);
    pathInfo.fs.root.getFile(pathInfo.path, {create: true}, function(fe) {
      fe.createWriter(onCreateWriter, reject);
    }, reject);

    function onCreateWriter(writer) {
      var blob = new Blob([content], {type:'text/plain'})
      // Discard arguments.
      writer.onwriteend = function() {
        resolve();
      }
      writer.onerror = reject;
      writer.write(blob);
    }
  });
};

/**
 * Remove a file from the filesystem. This does not work on directories.
 * @param {string} fileName The name of the file.
 */
FileManager.prototype.remove = function(fileName) {
  var self = this;
  return new Promise(function(resolve, reject) {
    if (!self.isInitialized) {
      reject(new Error(FileManager.FS_NOT_INITIALIZED));
      return;
    }

    var pathInfo = self.translatePath(fileName);
    pathInfo.fs.root.getFile(pathInfo.path, {}, function(fe) {
      fe.remove(resolve, reject);
    }, reject);
  });
};

/**
 * Create a directory.
 * @param {string} fileName The name of the directory.
 */
FileManager.prototype.makeDirectory = function(fileName) {
  var self = this;
  return new Promise(function(resolve, reject) {
    if (!self.isInitialized) {
      reject(new Error(FileManager.FS_NOT_INITIALIZED));
      return;
    }

    var pathInfo = self.translatePath(fileName);
    pathInfo.fs.root.getDirectory(pathInfo.path, {create: true}, function() {
      resolve();
    }, reject);
  });
};

/**
 * Remove a directory and its contents.
 * @param {string} fileName The name of the directory.
 */
FileManager.prototype.removeDirectory = function(fileName) {
  var self = this;
  return new Promise(function(resolve, reject) {
    if (!self.isInitialized) {
      reject(new Error(FileManager.FS_NOT_INITIALIZED));
      return;
    }

    var pathInfo = self.translatePath(fileName);
    pathInfo.fs.root.getDirectory(pathInfo.path, {}, function(dir) {
      dir.removeRecursively(function() {
        resolve();
      }, reject);
    }, reject);
  });
};
