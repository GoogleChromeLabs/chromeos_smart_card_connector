/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

NaClTerm.nmf = 'mongoose.nmf'

lib.rtdep('lib.f')

window.onload = function() {
  lib.init(function() {
    navigator.webkitPersistentStorage.requestQuota(1024*1024,
    function(bytes) {
      NaClTerm.init();
    },
    function(e) { console.log('Error', e); });
  });
};
