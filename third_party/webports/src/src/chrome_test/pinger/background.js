/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// A simple echo extension used to test extension proxying.


chrome.runtime.onConnectExternal.addListener(function(port) {
  port.onMessage.addListener(function(msg) {
    if (msg.name == 'ping') {
      msg.name = 'pong';
      port.postMessage(msg);
    }
  });
});
