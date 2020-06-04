/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('gdb.html', {
    'bounds': {
      'width': 400,
      'height': 500
    }
  });
});
