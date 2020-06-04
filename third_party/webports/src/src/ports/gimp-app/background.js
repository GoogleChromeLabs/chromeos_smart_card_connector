/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
var width = 1140;
var height = 810;

function onLaunched(launchData) {
  chrome.app.window.create('Xsdl.html', {
    width: width,
    height: height,
    minWidth: width,
    minHeight: height,
    maxWidth: width,
    maxHeight: height,
    id: 'Xsdl',
  });
}

chrome.app.runtime.onLaunched.addListener(onLaunched);
