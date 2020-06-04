/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';


TEST('ExtensionTest', 'testGetAllExtensions', function() {
  return chrometest.getAllExtensions().then(function(extensions) {
    ASSERT_EQ(3, extensions.length, 'there should be three extensions');
    var expected = [
      'Chrome Testing Extension',
      'Ping Test Extension',
      'TCP Interface',
    ];
    expected.sort();
    var actual = [];
    for (var i = 0; i < extensions.length; i++) {
      actual.push(extensions[i].name);
    }
    actual.sort();
    EXPECT_EQ(expected, actual, 'extensions should have the right names');
  });
});

TEST('ExtensionTest', 'testBuiltInPingPong', function() {
  var port = chrometest.newTestPort();
  var data = 'hickory dickory dock';
  port.postMessage({'name': 'ping', 'data': data});
  return port.wait().then(function(msg) {
    EXPECT_EQ('pong', msg.name, 'we should have gotten a pong');
    EXPECT_EQ(data, msg.data, 'we should get back what we sent');
    port.disconnect();
  });
});

TEST('ExtensionTest', 'testExtensionProxy', function() {
  var data = 'fee fi fo fum';
  var keptPort;
  return Promise.resolve().then(function() {
    return chrometest.proxyExtension('Ping Test Extension');
  }).then(function(port) {
    keptPort = port;
    keptPort.postMessage({'name': 'ping', 'data': data});
    return port.wait();
  }).then(function(msg) {
    EXPECT_EQ('pong', msg.name, 'we should have gotten a pong');
    EXPECT_EQ(data, msg.data, 'we should get back what we sent');
    keptPort.disconnect();
  });
});
