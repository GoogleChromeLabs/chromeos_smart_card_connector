/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

function domContentLoaded() {
  document.getElementById('romfile').addEventListener(
      'change', handleFileSelect, false);
}

function handleFileSelect(evt) {
  var file = evt.target.files[0];
  var reader = new FileReader();
  var result = null;

  console.log('reader.readAsArrayBuffer');
  reader.onloadend = onReadSuccess;
  reader.onerror = errorHandler;
  reader.readAsArrayBuffer(file);

  function onReadSuccess() {
    result = this.result;

    console.log('window.webkitRequestFileSystem');
    window.webkitRequestFileSystem(
        window.TEMPORARY, result.size, onRequestQuotaSuccess, errorHandler);
  }

  function onRequestQuotaSuccess(fs) {
    console.log('fs.root.getFile');
    fs.root.getFile(file.name, {create: true}, onGetFileSuccess, errorHandler);
  }

  function onGetFileSuccess(fileEntry) {
    console.log('fileEntry.createWriter');
    fileEntry.createWriter(onCreateWriterSuccess, errorHandler);
  }

  function onCreateWriterSuccess(writer) {
    console.log('writer.write');
    writer.onwriteend = onWriteSuccess;
    writer.onerror = errorHandler;
    var blob = new Blob([result]);
    writer.write(blob);
  }

  function onWriteSuccess() {
    console.log('startGame: ' + file.name);
    startGame(file.name);
  }
}

function errorHandler(error) {
  console.log('Error: ' + error);
}

function startGame(filename) {
  // Width and height in pixels of the SNES screen.
  var snesWidth = 256;
  var snesHeight = 239;

  var nacl = document.createElement('embed');
  nacl.setAttribute('id', 'snes9x');
  nacl.setAttribute('width', snesWidth * 2);  // Scale screen by 2x.
  nacl.setAttribute('height', snesHeight * 2);
  nacl.setAttribute('src', 'snes9x.nmf');
  nacl.setAttribute('type', 'application/x-nacl');
  nacl.setAttribute('ARG0',  'snes9x');
  nacl.setAttribute('ARG1',  filename);
  nacl.setAttribute('ARG2',  '-v1');  // Use "blocky" image scaling.

  // Remove previous embed element.
  document.getElementById('listener').innerHTML = '';
  document.getElementById('listener').appendChild(nacl);
}

document.addEventListener('DOMContentLoaded', domContentLoaded);
