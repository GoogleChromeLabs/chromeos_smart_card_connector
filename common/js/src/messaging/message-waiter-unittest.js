/**
 * @license
 * Copyright 2024 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

goog.require('GoogleSmartCard.MessageWaiter');
goog.require('goog.reflect');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.messaging.MockMessageChannel');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

/** @type {goog.testing.MockControl} */
let mockControl = null;
/** @type {goog.testing.messaging.MockMessageChannel} */
let channel = null;

goog.exportSymbol('testMessageWaiter', {
  'setUp': function() {
    mockControl = new goog.testing.MockControl();
    channel = new goog.testing.messaging.MockMessageChannel(mockControl);
  },

  'tearDown': function() {
    channel = null;
    mockControl.$verifyAll();
    mockControl = null;
  },

  'testBasic': async function() {
    const TYPE = 'foo';
    const VALUE = {};
    // Arrange.
    const promise = GSC.MessageWaiter.wait(channel, TYPE);
    // Act.
    channel.receive(TYPE, VALUE);
    // Assert.
    assertObjectEquals(await promise, VALUE);
  },

  'testUnrelatedMessage': async function() {
    const TYPE = 'foo';
    const VALUE = {'x': 1};
    const WRONG_TYPE = 'bar';
    const WRONG_VALUE = {'y': 2};
    // Arrange.
    const promise = GSC.MessageWaiter.wait(channel, TYPE);
    // Act.
    channel.receive(WRONG_TYPE, WRONG_VALUE);
    channel.receive(TYPE, VALUE);
    // Assert.
    assertObjectEquals(await promise, VALUE);
  },

  'testDisposal': async function() {
    // Arrange.
    const promise = GSC.MessageWaiter.wait(channel, 'foo');
    // Act. Hack: call the protected disposeInternal() method in order to
    // trigger the disposal notifications; MockMessageChannel.dispose() doesn't
    // call them.
    channel.dispose();
    channel[goog.reflect.objectProperty('disposeInternal', channel)]();
    // Assert.
    assertRejects(promise);
  },
});
});
