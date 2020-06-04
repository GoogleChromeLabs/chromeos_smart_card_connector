/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

function onLaunched(launchData) {
  chrome.app.window.create('index.html', {
    width: 640,
    height: 800,
  });
}

chrome.app.runtime.onLaunched.addListener(onLaunched);
