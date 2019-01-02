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

goog.require('GoogleSmartCard.DebugDump');
goog.require('GoogleSmartCard.Logging');
goog.require('GoogleSmartCard.MessageChannelPinging.PingResponder');
goog.require('GoogleSmartCard.MessageChannelPinging.Pinger');
goog.require('GoogleSmartCard.MockPort');
goog.require('GoogleSmartCard.PortMessageChannel');
goog.require('GoogleSmartCard.TypedMessage');
goog.require('goog.Promise');
goog.require('goog.testing.PropertyReplacer');
goog.require('goog.testing.jsunit');
goog.require('goog.testing.mockmatchers');

goog.setTestOnly();

goog.scope(function() {

/** @const */
var GSC = GoogleSmartCard;

/** @const */
var PortMessageChannel = GSC.PortMessageChannel;

/** @const */
var Pinger = GSC.MessageChannelPinging.Pinger;

/** @const */
var PingResponder = GSC.MessageChannelPinging.PingResponder;

/** @const */
var TypedMessage = GSC.TypedMessage;

/**
 * Mock matcher that matches only the ping messages.
 * @const
 */
var isPingMessageMockMatcher = new goog.testing.mockmatchers.ArgumentMatcher(
    function(value) {
      var typedMessage = TypedMessage.parseTypedMessage(value);
      return typedMessage && typedMessage.type == Pinger.SERVICE_NAME;
    }, 'isPingMessage');

/**
 * Sets up ping responding to ping messages posted through the mock port.
 * @param {!GSC.MockPort} mockPort
 */
function setUpPingRespondingForMockPort(mockPort) {
  var channelId = PingResponder.generateChannelId();
  mockPort.postMessage(isPingMessageMockMatcher).$atLeastOnce().$does(
      function(message) {
        var typedMessage = TypedMessage.parseTypedMessage(message);
        GSC.Logging.check(
            typedMessage && typedMessage.type == Pinger.SERVICE_NAME);
        var pingResponseMessage = new TypedMessage(
            PingResponder.SERVICE_NAME,
            PingResponder.createMessageData(channelId)).makeMessage();
        mockPort.fireOnMessage(pingResponseMessage);
      });
}

// Test that the channel is successfully established once a response to the ping
// request is received.
goog.exportSymbol('testPortMessageChannelEstablishing', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();

  var mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  function onPortMessageChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  var portMessageChannel = new GSC.PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);
  portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that the channel is not established and is disposed of when there are no
// responses to the ping requests received.
goog.exportSymbol('testPortMessageChannelFailureToEstablish', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();

  var mockPort = new GSC.MockPort('mock port');
  mockPort.postMessage(goog.testing.mockmatchers.isObject).$anyTimes();
  mockPort.postMessage.$replay();

  var propertyReplacer = new goog.testing.PropertyReplacer;
  propertyReplacer.set(Pinger, 'TIMEOUT_MILLISECONDS', 400);
  propertyReplacer.set(Pinger, 'INTERVAL_MILLISECONDS', 200);
  testCasePromiseResolver.promise.thenAlways(
      propertyReplacer.reset, propertyReplacer);

  function onPortMessageChannelEstablished() {
    testCasePromiseResolver.reject();
  }

  function onPortMessageChannelDisposed() {
    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  var portMessageChannel = new GSC.PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);
  portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that the messages sent through the port message channel are posted
// through the port with preserving the relative order.
goog.exportSymbol('testPortMessageChannelMessageSending', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();

  var TEST_MESSAGES = [
      new TypedMessage('service x', {}),
      new TypedMessage('service x', {some_data: 1}),
      new TypedMessage('service y', {})];

  var mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  for (let testMessage of TEST_MESSAGES) {
    mockPort.postMessage(new goog.testing.mockmatchers.ObjectEquals(
        testMessage.makeMessage())).$once();
  }
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    for (let testMessage of TEST_MESSAGES)
      portMessageChannel.send(testMessage.type, testMessage.data);

    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  function onPortMessageChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  var portMessageChannel = new GSC.PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);
  portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that the port message channel passes the messages received from the port
// to the correct services with preserving the relative order.
goog.exportSymbol('testPortMessageChannelMessageReceiving', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();

  var TEST_MESSAGES = [
      new TypedMessage('service x', {}),
      new TypedMessage('service x', {some_data: 1}),
      new TypedMessage('service y', {})];

  var mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    var receivedMessages = [];

    for (let testMessage of TEST_MESSAGES) {
      portMessageChannel.registerService(
          testMessage.type,
          function(messageData) {
            GSC.Logging.check(goog.isObject(messageData));
            goog.asserts.assert(goog.isObject(messageData));
            receivedMessages.push(new TypedMessage(
                testMessage.type, messageData));
          },
          true);
    }

    for (let testMessage of TEST_MESSAGES)
      mockPort.fireOnMessage(testMessage.makeMessage());

    assertObjectEquals(TEST_MESSAGES, receivedMessages);

    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  function onPortMessageChannelDisposed() {
    testCasePromiseResolver.reject();
  }

  var portMessageChannel = new GSC.PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);
  portMessageChannel.addOnDisposeCallback(onPortMessageChannelDisposed);

  return testCasePromiseResolver.promise;
});

// Test that the port message channel is disposed of when the port gets
// disconnected.
goog.exportSymbol('testPortMessageChannelDisconnection', function() {
  var testCasePromiseResolver = goog.Promise.withResolver();

  var mockPort = new GSC.MockPort('mock port');
  setUpPingRespondingForMockPort(mockPort);
  mockPort.postMessage.$replay();

  function onPortMessageChannelEstablished() {
    mockPort.getFakePort().disconnect();
    assert(portMessageChannel.isDisposed());

    assertThrows(function() {
      portMessageChannel.send('foo', {});
    });

    mockPort.postMessage.$verify();
    testCasePromiseResolver.resolve();
    mockPort.dispose();
  }

  var portMessageChannel = new GSC.PortMessageChannel(
      mockPort.getFakePort(), onPortMessageChannelEstablished);

  return testCasePromiseResolver.promise;
});

});  // goog.scope
