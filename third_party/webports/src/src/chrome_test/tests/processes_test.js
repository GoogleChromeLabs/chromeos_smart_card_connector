/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';


function ProcessesTest() {
  this.processes = null;
}
ProcessesTest.prototype = new chrometest.Test();

ProcessesTest.prototype.setUp = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return chrometest.Test.prototype.setUp.call(self);
  }).then(function() {
    return chrometest.getAllProcesses();
  }).then(function(processes) {
    self.processes = processes;
  });
};


TEST_F(ProcessesTest, 'testProcessTypes', function() {
  var browserCount = 0;
  var rendererCount = 0;
  var gpuCount = 0;
  var extensionCount = 0;
  for (var id in this.processes) {
    var process = this.processes[id];
    var type = process.type;
    if (type == 'browser') {
      browserCount++;
    } else if (type == 'renderer') {
      rendererCount++;
    } else if (type == 'gpu') {
      gpuCount++;
    } else if (type == 'extension') {
      extensionCount++;
    } else if (type == 'utility') {
      // There may be utility processes, ignore them.
    } else {
      EXPECT_TRUE(false, 'There should only be ' +
                  'browser, renderer, gpu, and extension, but got ' + type);
    }
  }
  EXPECT_EQ(1, browserCount, 'there should be one browser');
  EXPECT_EQ(1, gpuCount, 'there should be one gpu');
  EXPECT_GE(rendererCount, 1, 'there should be one or more renderers');
  EXPECT_GE(extensionCount, 2, 'there should be two or more extensions');
});

TEST_F(ProcessesTest, 'testExtensionTitles', function() {
  var hasChromeTest = false;
  var hasPingTest = false;
  for (var id in this.processes) {
    var process = this.processes[id];
    if (process.type == 'extension') {
      if (process.title == 'Extension: Chrome Testing Extension') {
        hasChromeTest = true
      } else if (process.title == 'Extension: Ping Test Extension') {
        hasPingTest = true;
      }
    }
  }
  EXPECT_TRUE(hasChromeTest, 'the chrometest extension should be present');
  EXPECT_TRUE(hasPingTest, 'the ping test extension should be present');
});
