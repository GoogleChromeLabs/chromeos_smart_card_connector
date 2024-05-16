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

goog.require('GoogleSmartCard.DelayedMessageChannel');
goog.require('goog.asserts');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.messaging.MockMessageChannel');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const TYPE_1 = 'one';
const DATA_1 = {
  'foo': 1
};
const TYPE_2 = 'two';
const DATA_2 = {
  'bar': 1
};

/** @type {goog.testing.MockControl} */
let mockControl = null;
/** @type {goog.testing.messaging.MockMessageChannel} */
let underlyingMock = null;
/** @type {GSC.DelayedMessageChannel} */
let delayed = null;
/** @type {!Array<!Array>} */
let receivedReplies = [];

goog.exportSymbol('testDelayedMessageChannel', {
  'setUp': function() {
    mockControl = new goog.testing.MockControl();
    underlyingMock = new goog.testing.messaging.MockMessageChannel(mockControl);
    /** @type {?} */ underlyingMock.send;

    delayed = new GSC.DelayedMessageChannel();
    delayed.registerDefaultService((serviceName, payload) => {
      receivedReplies.push([serviceName, payload]);
    });
  },

  'tearDown': function() {
    // Check all mock expectations are satisfied.
    mockControl.$verifyAll();
    mockControl = null;
    underlyingMock = null;
    if (delayed) {
      delayed.dispose();
      delayed = null;
    }
    receivedReplies = [];
  },

  // Messages are sent/received immediately if happening after `setReady()`.
  'testImmediateSendReceiveWhenReady': function() {
    // Arrange. Expect message 1 to be sent through the mock.
    underlyingMock.send(TYPE_1, DATA_1);
    underlyingMock.send.$replay();
    delayed.setUnderlyingChannel(underlyingMock);
    delayed.setReady();

    // Act. Send messages in both directions.
    delayed.send(TYPE_1, DATA_1);
    underlyingMock.receive(TYPE_2, DATA_2);

    // Assert.
    assertObjectEquals(receivedReplies, [[TYPE_2, DATA_2]]);
  },

  // Sent messages are delayed until `setReady()` is called.
  'testSendDelayedUntilReady': function() {
    // Hack to convince Closure Compiler that the variable is non-null below.
    goog.asserts.assert(underlyingMock);

    // Arrange. Call `send()` before setting mock expectations, to verify that
    // sending is delayed until `setReady()`.
    delayed.send(TYPE_1, DATA_1);
    delayed.setUnderlyingChannel(underlyingMock);
    delayed.send(TYPE_2, DATA_2);
    underlyingMock.send(TYPE_1, DATA_1);
    underlyingMock.send(TYPE_2, DATA_2);
    underlyingMock.send.$replay();

    // Act.
    delayed.setReady();

    // Assertions happen when verifying mocks in `tearDown()`.
  },

  // Message receipt happens immediately after `setUnderlyingChannel()`, even if
  // `setReady()` hasn't been called.
  'testReceiveImmediateBeforeReady': function() {
    // Hack to convince Closure Compiler that the variable is non-null below.
    goog.asserts.assert(underlyingMock);

    // Arrange.
    delayed.setUnderlyingChannel(underlyingMock);

    // Act.
    underlyingMock.receive(TYPE_1, DATA_1);

    // Assert.
    assertObjectEquals(receivedReplies, [[TYPE_1, DATA_1]]);
  },

  // Test sending the second message from inside the `send()` call for the first
  // message.
  'testReentrantSendWhenReady': function() {
    // Arrange. The mock will reentrantly send message 2 whenever message 1 is
    // being sent.
    underlyingMock.send(TYPE_1, DATA_1).$does(() => {
      delayed.send(TYPE_2, DATA_2);
    });
    underlyingMock.send(TYPE_2, DATA_2);
    underlyingMock.send.$replay();
    delayed.setUnderlyingChannel(underlyingMock);
    delayed.setReady();

    // Act.
    delayed.send(TYPE_1, DATA_1);

    // Assertions happen when verifying mocks in `tearDown()`.
  },

  // Same as above, but in the scenario when sending is delayed until
  // `setReady()` is called.
  'testReentrantSendBeforeReady': function() {
    // Arrange. The mock will reentrantly send message 2 whenever message 1 is
    // being sent.
    underlyingMock.send(TYPE_1, DATA_1).$does(() => {
      delayed.send(TYPE_2, DATA_2);
    });
    underlyingMock.send(TYPE_2, DATA_2);
    underlyingMock.send.$replay();
    delayed.setUnderlyingChannel(underlyingMock);
    delayed.send(TYPE_1, DATA_1);

    // Act.
    delayed.setReady();

    // Assertions happen when verifying mocks in `tearDown()`.
  },
});
});
