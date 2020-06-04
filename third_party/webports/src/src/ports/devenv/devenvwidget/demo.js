/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// This is an example use of devenvwidget.js.
function $(i) {
  return document.querySelector(i);
}
document.addEventListener('DOMContentLoaded', function() {
  var devenv = new DevEnvWidget({
    source: $('#source'),
    run: $('#run'),
    status: $('#status'),
    output: $('#output')
  });
});
