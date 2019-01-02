/** @license
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
goog.require('goog.async.Delay');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var SingleMessageBasedChannel = GSC.SingleMessageBasedChannel;

/** @const */
var Pinger = GSC.MessageChannelPinging.Pinger;

/** @const */
var PingResponder = GSC.MessageChannelPinging.PingResponder;

/** @const */
var TypedMessage = GSC.TypedMessage;

/** @const */
var EXTENSION_ID = 'DUMMY_EXTENSION_ID';

/** @const */
var TEST_MESSAGES = [
    new TypedMessage('service x', {}),
    new TypedMessage('service x', {some_data: 1}),
    new TypedMessage('service y', {})];

/** @const */
var channelId = PingResponder.generateChannelId();

/** @const */
var pingResponseMessage = new TypedMessage(
    PingResponder.SERVICE_NAME,
    PingResponder.createMessageData(channelId)).makeMessage();

/**
 * Mock matcher that matches strings.
 * @const
 */
var verifyChannelIdMatcher = new goog.testing.mockmatchers.ArgumentMatcher(
    function(value) {
      return value === EXTENSION_ID;
    }, 'verifyChannelId');

/**
 * Mock matcher that matches only the ping messages.
 * @const
 */
var isPingMessageMatcher = new goog.testing.mockmatchers.ArgumentMatcher(
    function(value) {
      var typedMessage = TypedMessage.parseTypedMessage(value);
      return typedMessage && typedMessage.type == Pinger.SERVICE_NAME;
    }, 'isPingMessage');

var globalChannel;

/**
 * Setup the mock for chrome.runtime.sendMessage.
 * @param {!goog.promise.Resolver} testCasePromiseResolver
 * @param {!goog.testing.PropertyReplacer} propertyReplacer
 */
function setupSendMessageMock(testCasePromiseResolver, propertyReplacer) {
  propertyReplacer.setPath(
      'chrome.runtime.sendMessage',
      goog.testing.createFunctionMock('sendMessage'));
  testCasePromiseResolver.promise.thenAlways(
      propertyReplacer.reset, propertyReplacer);
}

/**
 * Setup ping responding to messages going through chrome.runtime.sendMessage
 * (essentially intercepts the message and sends a response back).
 */
function setupSendMessagePingResponding() {
  // Type cast to prevent Closure compiler from complaining.
  /** @type {?} */ chrome.runtime.sendMessage;

  chrome.runtime.sendMessage(verifyChannelIdMatcher, isPingMessageMatcher)
      .$atLeastOnce().$does(
          function(channelId, message) {
            globalChannel.deliverMessage(pingResponseMessage);
          });
}

// Test that the channel is successfully established once a response to the ping
// request is received.
goog.exportSymbol('testSingleMessageBasedChannelEstablishing', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();
  var propertyReplacer = new goog.testing.PropertyReplacer;

  setupSendMessageMock(testCasePromiseResolver, propertyReplacer);
  setupSendMessagePingResponding();

  /** @type {?} */ chrome.runtime.sendMessage;

  chrome.runtime.sendMessage.$replay();

  function onChannelEstablished() {
    chrome.runtime.sendMessage.$verify();
    testCasePromiseResolver.resolve();
    globalChannel.dispose();
  }

  function onChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  globalChannel = new GSC.SingleMessageBasedChannel(
      EXTENSION_ID, onChannelEstablished);
  globalChannel.addOnDisposeCallback(onChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that the channel is not established and is disposed of when there are no
// responses to the ping requests received.
goog.exportSymbol('testSingleMessageBasedChannelFailureToEstablish',
                  function() {
  var testCasePromiseResolver = goog.Promise.withResolver();
  var propertyReplacer = new goog.testing.PropertyReplacer;

  setupSendMessageMock(testCasePromiseResolver, propertyReplacer);
  propertyReplacer.set(Pinger, 'TIMEOUT_MILLISECONDS', 400);
  propertyReplacer.set(Pinger, 'INTERVAL_MILLISECONDS', 200);

  /** @type {?} */ chrome.runtime.sendMessage;

  chrome.runtime.sendMessage(verifyChannelIdMatcher,
                             goog.testing.mockmatchers.isObject).$anyTimes();
  chrome.runtime.sendMessage.$replay();

  function onChannelEstablished() {
    testCasePromiseResolver.reject();
  }

  function onChannelDisposed() {
    chrome.runtime.sendMessage.$verify();
    testCasePromiseResolver.resolve();
    globalChannel.dispose();
  }

  globalChannel = new GSC.SingleMessageBasedChannel(
      EXTENSION_ID, onChannelEstablished);
  globalChannel.addOnDisposeCallback(onChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Verify that messages sent through the message channel are posted through
// with preserving the relative order.
goog.exportSymbol('testSingleMessageBasedChannelSending', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();
  var propertyReplacer = new goog.testing.PropertyReplacer;

  setupSendMessageMock(testCasePromiseResolver, propertyReplacer);
  setupSendMessagePingResponding();

  /** @type {?} */ chrome.runtime.sendMessage;

  for (let testMessage of TEST_MESSAGES) {
    chrome.runtime.sendMessage(
        verifyChannelIdMatcher, new goog.testing.mockmatchers.ObjectEquals(
            testMessage.makeMessage())).$once();
  }

  chrome.runtime.sendMessage.$replay();

  function onChannelEstablished() {
    for (let testMessage of TEST_MESSAGES) {
      globalChannel.send(testMessage.type, testMessage.data);
    }

    chrome.runtime.sendMessage.$verify();
    testCasePromiseResolver.resolve();
    globalChannel.dispose();
  }

  function onChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  globalChannel = new GSC.SingleMessageBasedChannel(
      EXTENSION_ID, onChannelEstablished);
  globalChannel.addOnDisposeCallback(onChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Verify that the message channel passes the messages received from the other
// side to the correct service while also preserving the relative order.
goog.exportSymbol('testSingleMessageBasedChannelReceiving', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();
  var propertyReplacer = new goog.testing.PropertyReplacer;

  setupSendMessageMock(testCasePromiseResolver, propertyReplacer);
  setupSendMessagePingResponding();

  /** @type {?} */ chrome.runtime.sendMessage;

  chrome.runtime.sendMessage.$replay();

  function onChannelEstablished() {
    var receivedMessages = [];

    for (let testMessage of TEST_MESSAGES) {
      globalChannel.registerService(
          testMessage.type,
          function(messageData) {
            GSC.Logging.check(goog.isObject(messageData));
            goog.asserts.assert(goog.isObject(messageData));
            receivedMessages.push(new TypedMessage(
                testMessage.type, messageData));
          },
          true);
    }

    for (let testMessage of TEST_MESSAGES) {
      globalChannel.deliverMessage(testMessage.makeMessage());
    }

    chrome.runtime.sendMessage.$verify();
    testCasePromiseResolver.resolve();
    globalChannel.dispose();
  }

  function onChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  globalChannel = new GSC.SingleMessageBasedChannel(
      EXTENSION_ID, onChannelEstablished);
  globalChannel.addOnDisposeCallback(onChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Verify that the message channel is disposed of when the underlying pinger
// stops receiving responses (but first the channel needs to be established).
goog.exportSymbol('testSingleMessageBasedChannelDisposing', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();
  var propertyReplacer = new goog.testing.PropertyReplacer;

  setupSendMessageMock(testCasePromiseResolver, propertyReplacer);
  propertyReplacer.set(Pinger, 'TIMEOUT_MILLISECONDS', 400);
  propertyReplacer.set(Pinger, 'INTERVAL_MILLISECONDS', 200);
  /** @type {?} */ chrome.runtime.sendMessage;

  var firstCallSeen = false;
  chrome.runtime.sendMessage(verifyChannelIdMatcher, isPingMessageMatcher)
      .$atLeastOnce().$does(
          function(message) {
            if (!firstCallSeen) {
              firstCallSeen = true;
              globalChannel.deliverMessage(pingResponseMessage);
            }
          });

  chrome.runtime.sendMessage.$replay();

  var channelEstablished = false;
  function onChannelEstablished() {
    channelEstablished = true;
  }

  function onChannelDisposed() {
    assert('Channel wasn\'t successfully established prior to failure',
           channelEstablished);
    assert(globalChannel.isDisposed());

    assertThrows(function() {
      globalChannel.send('foo', {});
    });

    chrome.runtime.sendMessage.$verify();
    testCasePromiseResolver.resolve();
    globalChannel.dispose();
  }

  globalChannel = new GSC.SingleMessageBasedChannel(
      EXTENSION_ID, onChannelEstablished);
  globalChannel.addOnDisposeCallback(onChannelDisposed);

  return testCasePromiseResolver.promise;
});

});  // goog.scope
