/**
 * @license
 * Copyright 2016 Google Inc. All Rights Reserved.
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

goog.require('GoogleSmartCard.MessageChannelPinging.PingResponder');
goog.require('GoogleSmartCard.MessageChannelPinging.Pinger');
goog.require('GoogleSmartCard.SingleMessageBasedChannel');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.Promise');
goog.require('goog.asserts');
goog.require('goog.promise.Resolver');
goog.require('goog.testing');
goog.require('goog.testing.MockControl');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.StrictMock');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

const GSC = GoogleSmartCard;

const SingleMessageBasedChannel = GSC.SingleMessageBasedChannel;

const Pinger = GSC.MessageChannelPinging.Pinger;

const PingResponder = GSC.MessageChannelPinging.PingResponder;

const TypedMessage = GSC.TypedMessage;

const EXTENSION_ID = 'DUMMY_EXTENSION_ID';

const TEST_MESSAGES = [
  new TypedMessage('service x', {}),
  new TypedMessage('service x', {some_data: 1}),
  new TypedMessage('service y', {})
];

const channelId = PingResponder.generateChannelId();

const pingResponseMessage =
    new TypedMessage(
        PingResponder.SERVICE_NAME, PingResponder.createMessageData(channelId))
        .makeMessage();

/** Mock matcher that matches strings. */
const verifyChannelIdMatcher =
    new goog.testing.mockmatchers.ArgumentMatcher(function(value) {
      return value === EXTENSION_ID;
    }, 'verifyChannelId');

/** Mock matcher that matches only the ping messages. */
const isPingMessageMatcher =
    new goog.testing.mockmatchers.ArgumentMatcher(function(value) {
      const typedMessage = TypedMessage.parseTypedMessage(value);
      return typedMessage && typedMessage.type == Pinger.SERVICE_NAME;
    }, 'isPingMessage');

const propertyReplacer = new goog.testing.PropertyReplacer();
/** @type {!goog.testing.MockControl|undefined} */
let mockControl;
/** @type {!SingleMessageBasedChannel|undefined} */
let testChannel;
/** @type {!goog.testing.StrictMock|undefined} */
let mockedSendMessage;

/**
 * Sets the chrome.runtime.sendMessage() mock to expect it'll be called with a
 * "ping" message, and will respond with `pingResponseMessage`.
 */
function willRespondToPing() {
  chrome.runtime.sendMessage(verifyChannelIdMatcher, isPingMessageMatcher);
  mockedSendMessage.$atLeastOnce().$does(function(channelId, message) {
    testChannel.deliverMessage(pingResponseMessage);
  });
}

// Helper class that allows to wait for the callback call by awaiting a promise.
class Waiter {
  constructor() {
    /** @type {!Function} */
    this.callback = () => {};
    this.promise = new Promise((resolve, reject) => {
      this.callback = resolve;
    });
  }
}

goog.exportSymbol('testSingleMessageBasedChannel', {

  'setUp': function() {
    mockControl = new goog.testing.MockControl();
    // Mock chrome.runtime.sendMessage(). Cast is to work around the issue that
    // the return type of createFunctionMock() is too generic (as it depends on
    // the value of the optional second argument).
    mockedSendMessage = /** @type {!goog.testing.StrictMock} */ (
        mockControl.createFunctionMock('sendMessage'));
    propertyReplacer.setPath('chrome.runtime.sendMessage', mockedSendMessage);
    // Override the pinger's default timeouts, to prevent the test from hitting
    // the test framework's limit on the single test execution.
    Pinger.overrideTimeoutForTesting(400);
    Pinger.overrideIntervalForTesting(200);
  },

  'tearDown': function() {
    // Check all mock expectations are satisfied.
    mockControl.$verifyAll();
    // Clean up.
    if (testChannel) {
      testChannel.dispose();
      testChannel = undefined;
    }
    propertyReplacer.reset();
    // Restore pinger's default timeouts.
    Pinger.overrideTimeoutForTesting(null);
    Pinger.overrideIntervalForTesting(null);
  },

  // Test that the channel is successfully established once a response to the
  // ping request is received.
  'testEstablishing': async function() {
    // Arrange: set mock expectations.
    willRespondToPing();
    mockControl.$replayAll();

    // Act: instantiate a channel and wait till it switches into the
    // "established" state.
    const waiter = new Waiter();
    testChannel = new SingleMessageBasedChannel(
        EXTENSION_ID, /* opt_onEstablished= */ waiter.callback);
    await waiter.promise;

    // Assert.
    assertFalse(testChannel.isDisposed());
  },

  // Test that the channel is not established and is disposed of when there are
  // no responses to the ping requests received.
  'testFailureToEstablish': async function() {
    // Arrange: set mock expectations that chrome.runtime.sendMessage() will be
    // called some number of times, each time with the correct channelId field.
    chrome.runtime.sendMessage(
        verifyChannelIdMatcher, goog.testing.mockmatchers.isObject);
    mockedSendMessage.$anyTimes();
    mockControl.$replayAll();

    // Act: create a channel and wait until it switches into the "disposed"
    // state.
    const waiter = new Waiter();
    testChannel = new SingleMessageBasedChannel(
        EXTENSION_ID, /* opt_onEstablished= */
        () => {
          fail('Unexpectedly established');
        });
    testChannel.addOnDisposeCallback(waiter.callback);
    await waiter.promise;
  },

  // Verify that messages sent through the message channel are posted through
  // with preserving the relative order.
  'testDataSending': async function() {
    // Arrange: set mock expectations that chrome.runtime.sendMessage() will be
    // called with ping messages, and also once for each of `TEST_MESSAGES`, all
    // with a correct channelId.
    willRespondToPing();
    for (const testMessage of TEST_MESSAGES) {
      chrome.runtime.sendMessage(
          verifyChannelIdMatcher,
          new goog.testing.mockmatchers.ObjectEquals(
              testMessage.makeMessage()));
      mockedSendMessage.$once();
    }
    mockControl.$replayAll();

    // Act: create a channel, and wait until it switches into the "established"
    // state.
    const waiter = new Waiter();
    testChannel = new SingleMessageBasedChannel(
        EXTENSION_ID, /* opt_onEstablished= */ waiter.callback);
    await waiter.promise;
    // Send test messages through the channel.
    for (const testMessage of TEST_MESSAGES)
      testChannel.send(testMessage.type, testMessage.data);

    // Assert.
    assertFalse(testChannel.isDisposed());
  },

  // Test that array buffers in sent messages are substituted with byte arrays.
  'testArrayBufferSending': async function() {
    // Arrange: set mock expectations that chrome.runtime.sendMessage() will be
    // called with ping messages, and also once with the given array and the
    // expected channelId.
    const MESSAGE_TYPE = 'foo';
    const MESSAGE_DATA = {x: (new Uint8Array([1, 255])).buffer};
    const EXPECTED_TRANSMITTED_DATA = {x: [1, 255]};
    const EXPECTED_TRANSMITTED_MESSAGE =
        new TypedMessage(MESSAGE_TYPE, EXPECTED_TRANSMITTED_DATA);
    willRespondToPing();
    chrome.runtime.sendMessage(
        verifyChannelIdMatcher,
        new goog.testing.mockmatchers.ObjectEquals(
            EXPECTED_TRANSMITTED_MESSAGE.makeMessage()));
    mockedSendMessage.$once();
    mockControl.$replayAll();

    // Act: create a channel, and wait until it switches into the "established"
    // state.
    const waiter = new Waiter();
    testChannel = new SingleMessageBasedChannel(
        EXTENSION_ID, /* opt_onEstablished= */ waiter.callback);
    await waiter.promise;
    // Send a test message with an array buffer through the channel.
    testChannel.send(MESSAGE_TYPE, MESSAGE_DATA);

    // Assert.
    assertFalse(testChannel.isDisposed());
  },

  // Verify that the message channel passes the messages received from the other
  // side to the correct service while also preserving the relative order.
  'testDataReceipt': async function() {
    // Arrange: set up mock expectations that chrome.runtime.sendMessage() will
    // be called with ping messages and reply to them.
    willRespondToPing();
    mockControl.$replayAll();

    // Act: create a channel, and wait until it switches into the "established"
    // state.
    const waiter = new Waiter();
    testChannel = new SingleMessageBasedChannel(
        EXTENSION_ID, /* opt_onEstablished= */ waiter.callback);
    await waiter.promise;
    // Set up an observer for incoming messages.
    const receivedMessages = [];
    for (const testMessage of TEST_MESSAGES) {
      testChannel.registerService(testMessage.type, function(messageData) {
        goog.asserts.assert(goog.isObject(messageData));
        receivedMessages.push(new TypedMessage(testMessage.type, messageData));
      }, /* opt_objectPayload= */ true);
    }
    // Simulate incoming test messages.
    for (const testMessage of TEST_MESSAGES)
      testChannel.deliverMessage(testMessage.makeMessage());

    // Assert: check all messages were received and in the correct order.
    assertEquals(receivedMessages.length, TEST_MESSAGES.length);
    for (let i = 0; i < TEST_MESSAGES.length; ++i) {
      assertEquals(receivedMessages[i].type, TEST_MESSAGES[i].type);
      assertEquals(receivedMessages[i].data, TEST_MESSAGES[i].data);
    }
    assertFalse(testChannel.isDisposed());
  },

  // Verify that the message channel is disposed of when the underlying pinger
  // stops receiving responses (but first the channel needs to be established).
  'testDisposal': async function() {
    // Arrange: set up the expectation that chrome.runtime.sendMessage() will be
    // called multiple times with ping messages, however only the first call
    // will reply.
    let firstCallSeen = false;
    chrome.runtime.sendMessage(verifyChannelIdMatcher, isPingMessageMatcher);
    mockedSendMessage.$atLeastOnce().$does(function(message) {
      if (!firstCallSeen) {
        firstCallSeen = true;
        testChannel.deliverMessage(pingResponseMessage);
      }
    });
    mockControl.$replayAll();

    // Act: create a channel, and wait until it switches into the "established"
    // state.
    const waiter = new Waiter();
    testChannel = new SingleMessageBasedChannel(
        EXTENSION_ID, /* opt_onEstablished= */ waiter.callback);
    await waiter.promise;
    // Wait until the channel switches into the "disposed" state.
    const disposalWaiter = new Waiter();
    testChannel.addOnDisposeCallback(disposalWaiter.callback);
    await disposalWaiter.promise;

    // Assert: it's not allowed to send messages through a disposed channel.
    assertThrows(function() {
      testChannel.send('foo', {});
    });
  },

});
});  // goog.scope
