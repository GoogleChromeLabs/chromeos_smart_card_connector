/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

NaClTerm.nmf = 'gforth.nmf'

function onFailure() {
  console.log('Failed to allocate space!\n');
  NaClTerm.init();
}

function requestTemporary(amount, success, failure) {
  navigator.webkitTemporaryStorage.requestQuota(amount, function(bytes) {
    window.webkitRequestFileSystem(
        window.TEMPORARY, bytes, success, failure);
  }, failure);
}

function requestPersistent(amount, success, failure) {
  navigator.webkitPersistentStorage.requestQuota(amount, function(bytes) {
    window.webkitRequestFileSystem(
        window.PERSISTENT, bytes, success, failure);
  }, failure);
}

function requestStorage(success, failure) {
  requestTemporary(1024 * 1024, function() {
    requestPersistent(1024 * 1024, success, failure);
  }, failure);
}

function onInit() {
  requestStorage(NaClTerm.init, onFailure);
}

window.onload = function() {
  lib.init(function() {
    onInit();
  });
};
