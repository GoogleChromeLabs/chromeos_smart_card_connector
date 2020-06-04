/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

TEST('UserAgentTest', 'testInTestMode', function() {
  var parts = navigator.userAgent.split(' ');
  ASSERT_EQ(2, parts.length, 'agent has 2 sections');
  EXPECT_EQ('ChromeTestAgent', parts[0].split('/')[0],
            'should have test agent string');
  EXPECT_EQ(32, parts[0].split('/')[1].length,
            'should be a 32 digit extension id');
  EXPECT_EQ('Chrome', parts[1].split('/')[0],
            'must have chrome agent string');
  EXPECT_TRUE(/^\d+$/.test(parts[1].split('/')[1]),
              'must have a version number (for hterm)');
});
